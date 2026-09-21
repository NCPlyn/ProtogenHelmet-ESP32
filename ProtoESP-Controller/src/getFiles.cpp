#include "getFiles.h"
#include <LittleFS.h>
#include "configVars.h"

/* static */ bool getfilesProper = true;
/* static */ String getfilesCache = "";

//--------------------------------//getting stored anims names and count
void getFilesFunc() {
  if(!getfilesProper) return;
  getfilesCache = "";
  totalAnims = 0;
  File root = LittleFS.open("/anims");
  File file = root.openNextFile();
  while(file){
    availAnims[totalAnims] = String(file.name());
    getfilesCache += availAnims[totalAnims] + ";";
    totalAnims++;
    file = root.openNextFile();
  }
  getfilesProper = false;
}

/* Security-enhanced interfaces.
 * NOTE: Do not inline the functions below */

void getFilesReset() { getfilesProper = true; }

const String & getFilesCache() { return getfilesCache; }
