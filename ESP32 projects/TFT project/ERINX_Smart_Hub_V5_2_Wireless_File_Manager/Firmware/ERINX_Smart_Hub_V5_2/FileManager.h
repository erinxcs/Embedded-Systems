#pragma once
// Included near the end of the .ino, after the TFT/SD/Wi-Fi page definitions.
// Requires WebServer.h and FilePortal.h at the top of the main sketch.

static const uint32_t FILE_MAX_PHOTO = 6UL * 1024UL * 1024UL;
static const uint32_t FILE_MAX_VIDEO = 60UL * 1024UL * 1024UL;
static const char* FILE_TEMP_PHOTO = "/photos/.erinx_incoming";
static const char* FILE_TEMP_VIDEO = "/videos/.erinx_incoming";

static WebServer fileWeb(80);
static uint8_t fileBadPins = 0;
static unsigned long fileLockUntilMs = 0;
static fs::File fileReceiving;
static String filePendingName;
static String filePendingTemp;
static String filePendingError;
static bool fileUploadFinished = false;
static bool fileLibraryChanged = false;
static uint32_t fileReceivingBytes = 0;

static void fileSend(int code, const String& json) {
  fileWeb.sendHeader("Cache-Control", "no-store");
  fileWeb.sendHeader("X-Content-Type-Options", "nosniff");
  fileWeb.sendHeader("X-Frame-Options", "DENY");
  fileWeb.send(code, "application/json; charset=utf-8", json);
}
static void fileFail(int code, const String& reason) {
  JsonDocument d;
  d["error"] = reason;
  String j; serializeJson(d,j);
  fileSend(code,j);
}
static bool fileAuthed() {
  if ((int32_t)(millis() - fileLockUntilMs) < 0) {
    fileFail(429,"Too many wrong PIN attempts. Try again in 30 seconds.");
    return false;
  }
  if (!fileWeb.authenticate("admin",filePin.c_str())) {
    if (++fileBadPins >= 8) { fileBadPins = 0; fileLockUntilMs = millis() + 30000UL; }
    fileFail(401,"Wrong access PIN. Check Settings on the Smart Hub.");
    return false;
  }
  fileBadPins = 0;
  return true;
}
static bool fileSDReady() {
  if (!sdReady) {fileFail(503,"MicroSD not ready. Restart the Smart Hub with a FAT32 SD card.");return false;}
  return true;
}
static bool filePlaybackBusy() {
  if (currentPage == PAGE_VIDEO_PLAYER) {
    fileFail(409,"Leave the VIDEO PLAYER on the touchscreen before managing SD files.");
    return true;
  }
  return false;
}
static bool fileSafeName(const String& s, bool video) {
  if (s.length() < 5 || s.length() > 62 || s[0] == '.' || s.indexOf("..") >= 0) return false;
  for (size_t i=0; i<s.length(); ++i) {
    char c=s[i];
    if (!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||
          c=='_'||c=='-'||c=='.'||c==' ')) return false;
  }
  String lower=s;lower.toLowerCase();
  return video ? lower.endsWith(".evm") : (lower.endsWith(".jpg") || lower.endsWith(".jpeg"));
}
static String fileBaseName(String input) {
  input.replace('\\','/');
  int n=input.lastIndexOf('/');
  return n>=0?input.substring(n+1):input;
}
static String filePath(bool video,const String& name) {
  return String(video?"/videos/":"/photos/") + name;
}
static bool fileValidateBytes(bool video) {
  fs::File f=SD_MMC.open(filePendingTemp,FILE_READ);
  if(!f)return false;
  bool valid=false;
  if(video) {
    uint8_t h[16]={0};
    if(f.size()>=20 && f.read(h,16)==16 && memcmp(h,"ERXV",4)==0) {
      uint16_t w=h[4]|(h[5]<<8), ht=h[6]|(h[7]<<8), fps=h[8]|(h[9]<<8);
      uint32_t frames=(uint32_t)h[12]|((uint32_t)h[13]<<8)|((uint32_t)h[14]<<16)|((uint32_t)h[15]<<24);
      valid = w>0 && w<=320 && ht>0 && ht<=480 && fps>=1 && fps<=15 && frames>0 && frames<=100000;
    }
  } else {
    if(f.size()>=4) {
      int a=f.read(),b=f.read();
      if(f.seek(f.size()-2)) { int z=f.read(),q=f.read(); valid=(a==0xff && b==0xd8 && z==0xff && q==0xd9); }
    }
  }
  f.close();return valid;
}
static void fileCleanupUpload() {
  if(fileReceiving)fileReceiving.close();
  if(filePendingTemp.length() && sdReady) SD_MMC.remove(filePendingTemp);
}
static void fileUploadChunk(bool video) {
  HTTPUpload &u=fileWeb.upload();
  if(u.status==UPLOAD_FILE_START) {
    fileCleanupUpload();filePendingName="";filePendingTemp="";filePendingError="";
    fileReceivingBytes=0;fileUploadFinished=false;
    // Authenticate before accepting even the first byte.
    if(!fileWeb.authenticate("admin",filePin.c_str())) {filePendingError="Unauthorized";return;}
    if(!sdReady){filePendingError="No microSD card";return;}
    if(currentPage==PAGE_VIDEO_PLAYER){filePendingError="Leave Video Player before uploading";return;}
    filePendingName=fileBaseName(u.filename);
    if(!fileSafeName(filePendingName,video)){filePendingError="Use a valid JPG/JPEG or EVM name (ASCII letters/numbers/spaces/_/- only)";return;}
    String destination=filePath(video,filePendingName);
    if(SD_MMC.exists(destination)){filePendingError="File already exists. Rename the file before uploading.";return;}
    const char *folder=video?"/videos":"/photos";
    if(!SD_MMC.exists(folder) && !SD_MMC.mkdir(folder)){filePendingError="Cannot create media folder";return;}
    filePendingTemp=video?FILE_TEMP_VIDEO:FILE_TEMP_PHOTO;
    SD_MMC.remove(filePendingTemp);
    fileReceiving=SD_MMC.open(filePendingTemp,FILE_WRITE);
    if(!fileReceiving)filePendingError="Cannot write to microSD";
  } else if(u.status==UPLOAD_FILE_WRITE) {
    if(filePendingError.length() || !fileReceiving)return;
    uint32_t limit=video?FILE_MAX_VIDEO:FILE_MAX_PHOTO;
    if(u.currentSize>limit || fileReceivingBytes>limit-u.currentSize){filePendingError="File exceeds upload size limit";fileCleanupUpload();return;}
    size_t n=fileReceiving.write(u.buf,u.currentSize);
    if(n!=u.currentSize){filePendingError="SD write failed or card full";fileCleanupUpload();return;}
    fileReceivingBytes+=(uint32_t)n;
  } else if(u.status==UPLOAD_FILE_END) {
    if(fileReceiving){fileReceiving.flush();fileReceiving.close();}
    fileUploadFinished=true;
    if(filePendingError.length())fileCleanupUpload();
  } else if(u.status==UPLOAD_FILE_ABORTED) {
    filePendingError="Upload cancelled";fileCleanupUpload();
  }
}
static void fileFinishUpload(bool video) {
  if(!fileAuthed()){fileCleanupUpload();return;}
  if(!fileSDReady()){fileCleanupUpload();return;}
  if(filePlaybackBusy()){fileCleanupUpload();return;}
  if(filePendingError.length()){fileFail(400,filePendingError);fileCleanupUpload();return;}
  if(!fileUploadFinished || !filePendingName.length() || fileReceivingBytes==0) {
    fileFail(400,"No complete file received");fileCleanupUpload();return;
  }
  if(!fileValidateBytes(video)){fileFail(400,"Invalid file: expected baseline JPEG or ERINX EVM video");fileCleanupUpload();return;}
  String dest=filePath(video,filePendingName);
  if(SD_MMC.exists(dest) || !SD_MMC.rename(filePendingTemp,dest)) {
    fileFail(500,"Unable to save media (duplicate filename or SD error)");fileCleanupUpload();return;
  }
  filePendingTemp="";fileUploadFinished=false;fileLibraryChanged=true;
  JsonDocument d;d["ok"]=true;d["name"]=filePendingName;d["size"]=fileReceivingBytes;
  String j;serializeJson(d,j);fileSend(200,j);
  Serial.printf("WEB: uploaded %s (%lu bytes)\n",dest.c_str(),(unsigned long)fileReceivingBytes);
}
static void fileStatus() {
  if(!fileAuthed()||!fileSDReady())return;
  JsonDocument d;
  d["connected"]=WiFi.status()==WL_CONNECTED;
  d["ip"]=WiFi.localIP().toString();
  d["total"]=(uint64_t)SD_MMC.totalBytes();
  d["used"]=(uint64_t)SD_MMC.usedBytes();
  String j;serializeJson(d,j);fileSend(200,j);
}
static void fileList() {
  if(!fileAuthed()||!fileSDReady()||filePlaybackBusy())return;
  JsonDocument d;JsonObject lists=d["files"].to<JsonObject>();
  for(uint8_t k=0;k<2;k++){
    bool video=k==1;JsonArray arr=lists[video?"videos":"photos"].to<JsonArray>();
    fs::File dir=SD_MMC.open(video?"/videos":"/photos",FILE_READ);
    if(!dir||!dir.isDirectory()){if(dir)dir.close();continue;}
    fs::File f=dir.openNextFile();int count=0;
    while(f && count<80) {
      if(!f.isDirectory()){
        String name=fileBaseName(String(f.name()));
        if(fileSafeName(name,video)){
          JsonObject item=arr.add<JsonObject>();
          item["name"]=name;item["size"]=(uint32_t)f.size();++count;
        }
      }
      f.close();f=dir.openNextFile();
    }
    if(f) f.close();
    dir.close();
  }
  String j;serializeJson(d,j);fileSend(200,j);
}
static void fileDelete() {
  if(!fileAuthed()||!fileSDReady()||filePlaybackBusy())return;
  bool video=fileWeb.arg("type")=="videos";
  if(fileWeb.arg("type")!="photos" && !video){fileFail(400,"Invalid file type");return;}
  String name=fileWeb.arg("name");
  if(!fileSafeName(name,video)){fileFail(400,"Invalid filename");return;}
  String path=filePath(video,name);
  if(!SD_MMC.exists(path)){fileFail(404,"File not found");return;}
  if(!SD_MMC.remove(path)){fileFail(500,"Cannot delete file from SD");return;}
  fileLibraryChanged=true;fileSend(200,"{\"ok\":true}");
  Serial.printf("WEB: deleted %s\n",path.c_str());
}
static void filePhoto() {
  if(!fileAuthed()||!fileSDReady()||filePlaybackBusy())return;
  String name=fileWeb.arg("name");
  if(!fileSafeName(name,false)){fileFail(400,"Invalid photo filename");return;}
  fs::File f=SD_MMC.open(filePath(false,name),FILE_READ);
  if(!f){fileFail(404,"Photo not found");return;}
  fileWeb.sendHeader("Cache-Control","no-store");
  fileWeb.sendHeader("X-Content-Type-Options","nosniff");
  fileWeb.streamFile(f,"image/jpeg");f.close();
}
static void fileStartWeb() {
  fileWeb.on("/",HTTP_GET,[](){
    fileWeb.sendHeader("Cache-Control","no-store");
    fileWeb.sendHeader("X-Frame-Options","DENY");
    fileWeb.send_P(200,"text/html; charset=utf-8",ERINX_FILE_PORTAL_HTML);
  });
  fileWeb.on("/api/status",HTTP_GET,fileStatus);
  fileWeb.on("/api/files",HTTP_GET,fileList);
  fileWeb.on("/api/photo",HTTP_GET,filePhoto);
  fileWeb.on("/api/delete",HTTP_POST,fileDelete);
  fileWeb.on("/api/upload/photos",HTTP_POST,[](){fileFinishUpload(false);},[](){fileUploadChunk(false);});
  fileWeb.on("/api/upload/videos",HTTP_POST,[](){fileFinishUpload(true);},[](){fileUploadChunk(true);});
  fileWeb.onNotFound([](){fileFail(404,"Route not found");});
}
static void fileWebInit() {
  char pin[7];snprintf(pin,sizeof(pin),"%06lu",(unsigned long)(100000UL+(esp_random()%900000UL)));
  filePin=pin;
  fileStartWeb();
  if(sdReady){ SD_MMC.remove(FILE_TEMP_PHOTO);SD_MMC.remove(FILE_TEMP_VIDEO); }
  Serial.println("ERINX File Manager ready. Open Settings for device IP and access PIN.");
}
static void fileWebTick() {
  bool linked=WiFi.status()==WL_CONNECTED;
  if(linked && !fileWebOnline){ fileWeb.begin();fileWebOnline=true;Serial.println("ERINX File Manager web server started"); }
  if(!linked && fileWebOnline){ fileWeb.stop();fileWebOnline=false;fileCleanupUpload(); }
  if(fileWebOnline) fileWeb.handleClient();
  if(fileLibraryChanged && currentPage!=PAGE_VIDEO_PLAYER){
    fileLibraryChanged=false;
    if(currentPage==PAGE_GALLERY){refreshPhotoList();drawGallery();}
    else if(currentPage==PAGE_VIDEO){refreshVideoList();drawVideoList();}
  }
}
