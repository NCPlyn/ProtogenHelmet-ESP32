#pragma once
#include <Arduino.h>

//--------------------------------//Config vars
/* Warning:
 * the vars below will become static in the future's Security update
 * please do not directly change the vars below
 * as it'll be invalid in the future
 * instead, use the security-enhanced interfaces provided below.
 */
extern bool getfilesProper;
extern String getfilesCache;

void getFilesFunc(); //getting stored anims names and count

/* Security-enhanced interfaces.
 * NOTE: Do not inline the functions below */

void getFilesReset(); // reset getfilesProper to true
// to get the value of getfilesCache, use the function below
const String & getFilesCache();
