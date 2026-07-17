/*
 splitString (StringSplitter) has been replaced by strToArray. Parameters are the same
 
  2025-04-19 - Changed:
                  sourceIdtoName(source id) to srcIdToNm(source id)
 
 */

#include <Arduino.h>
#include <ArduinoTrace.h>
#include <myStuff.h>
#include <commonItems_ESP32.h>
#include <myText.h>

/*---------------  CONVERT STRING TO BOOLEAN  ---------------*/

bool strToBool(String s) {
  if(s == "1"     ||
     s == "true"  ||
     s == "TRUE"  ||
     s == "True"  ||
     s == "yes"   ||
     s == "YES"   ||
     s == "on"    ||
     s == "On"    ||
     s == "ON")   return true;
  else return false;
}


/*---------------  STRING TO ARRAY USING A DELINIATING CHAR  ---------------*/
 
int strToArray(const String &s, char c, int limit, String arr[]) {
  int count = countOccurrencesOfChar(s, c) + 1;                                 // fix for no elements, ie return input string
  if(count <= 1) {                                                              // if the count is les than or equal to 1 then
    count = 1;                                                                  // set count to 1
    arr[0] = s;                                                                 // and load the string into arr[0]
    return count;                                                               // and return count
  }
  
  if(count > limit) {                                                           // if more than the limit items were found set count to limit
    mySP("ERROR: To many occurrences of '" + String(c) + "' were found in str" +
         s + ".  count was changed to the limit: " + String(limit) + "\n",FN,LN);
    count = limit;
  }
  
  int startIndex = 0;                                   // whereto start each sunstring
  int endIndex = s.indexOf(c);                          // where to end each substring
  int i = 0;                                            // array index counter
  
  while(endIndex != -1) {                               // indexOf returns a -1 when no matches are fould so loop until this is received
    arr[i++] = s.substring(startIndex, endIndex);       // create the array element for this array index
    startIndex = endIndex + 1;                          // move past the the last deliniator by 1 char fo rthe new start position
    endIndex = s.indexOf(c, startIndex);                // get the new end position
  }
  arr[i++] = s.substring(startIndex);         // create the final array element
  return i;                                             // return the count of array elements
}


/*---------------  PUT THE CONTENTS OF A STRING INTO AN ARRAY DELINEATED BY CHAR  ---------------*

int strToArray(String s, char c, int limit, String arr[]) {
  int count = countOccurrencesOfChar(s, c) + 1;                                 // fix for no elements, ie return input string
  if(count <= 1) {                                                              // if the count is les than or equal to 1 then
    count = 1;                                                                  // set count to 1
    arr[0] = s;                                                                 // and load the string into arr[0]
    return count;                                                               // and return count
  }
  
  if(count > limit) {                                                           // if more than the limit items were found set count to limit
    mySP("ERROR: To many occurrences of '" + String(c) + "' were found in str" +
         s + ".  count was changed to the limit: " + String(limit) + "\n",FN,LN);
    count = limit;
  }
    
  String d = String(c);                                                         // convert the char c to a string (array delineator)
  String first;
  String second = s;
  
  int current = 0;                                                              // create a counter
  while(second.indexOf(d) > -1) {                                               // while not indexed to an occurrence of the d char
    if(current >= (count -1)) {                                                 // if indexed past the limit then break out of the loop
      break;
    }

    for (int i = 0; i < second.length(); i++) {                                 // for each char in second
       if (second.substring(i, i + 1) == d) {                                    // if this char is a array delineator
        first = second.substring(0, i);                                         // put from char 0 to char i into string first
        second = second.substring(i + 1);                                       // and remove these chars from string second
        if(first.length() > 0)                                                // if the length of string first is greater than zero
          arr[current++] = first;                                               // put the string first into the array at i and increment the counter 'current'
        break;                                                                  // and exit the for loop (back to the while loop
      }
    }
  }
  if(second.length() > 0)                                                       // if there is anything left in 'second'
    arr[current++] = second;                                                    // put it into the next array element
  return count;
}

 /*---------------  STRING ARRAY TO STRING  ---------------*/
 
String strArrToStr(String arr[], int len) {
   if (len <= 0 || arr == nullptr) return "";         // Protect against empty arrays or invalid bounds
   int totalBytesNeeded = 0;
   for (int i = 0; i < len; i++) {                    // Calculate the exact memory box size needed
     totalBytesNeeded += arr[i].length();
   }
  String s;
   s.reserve(totalBytesNeeded);                       // Allocate the memory ONCE from the heap
   for (int i = 0; i < len; i++) {                    // Fill the box smoothly without any re-allocation steps
     s += arr[i];
   }
   return s;
 }

/*---------------  STRING ARRAY TO STRING  ---------------*

String strArrToStr(String arr[], int len) {
  String s = "";
  for(int i = 0; i < len; i++) {
    s += arr[i];
  }
  return s;
}

 /*---------------  uint8_t ARRAY TO STRING  ---------------*/
 
String uint8ArrToStr(const uint8_t a[], int len, const String &seperator) {
   // 1. Guard Gate: Protect against invalid lengths or zero addresses
   if (len <= 0 || a == nullptr) return "";
   int maxBytesNeeded = len * (3 + seperator.length());       // Worst-Case Sizing: A uint8_t is 1-3 digits. (3 + separator length) * len.
   String s = "";
   s.reserve(maxBytesNeeded);                                 // Allocates a single box on the heap once!
   for (int i = 0; i < len; i++) {                            // Populate smoothly using native character and integer appending
     s += a[i];                                               // Direct integer appending bypasses temporary String constructor objects
     if (i < len - 1) {                                       // Inline separator calculation removes the need to use .substring() at the end
       s += seperator;
     }
   }
   return s;                                                  // Return Value Optimization (RVO) passes this box back to Function A with 0 copies!
 }

/*---------------  uint8_t ARRAY TO STRING  ---------------*

String uint8ArrToStr(uint8_t a[], int len, String seperator) {
  String s = "";
  for(int i = 0; i < len; i++) {
    s += String(a[i]) + seperator;
  }
  s = s.substring(0, s.length() - 1);
  return s;
}

/*---------------  STRING TO unt_8 ARRAY  ---------------*/

void strToUINT_8Arr(String s, char c, uint8_t arr[], int len) {
  String strArr[len];
  int count = strToArray(s, ',',len,  strArr);                               // convert the string into a String array
  if(count != len) {
    mySP("ERROR: converting an string to a unit8 array and the "
         "element count is wrong.  Supposed to be '" + String(len) +
         "' but it appears to be '" + String(count) +
         "'. Please cause this to be fixed.\n", FN, LN);
    return;
  }
  strToArray(s, c, len, strArr);
  for(int i = 0; i < len; i++) {
    arr[i] = (uint8_t)strArr[i].toInt();
  }
}

/*---------------  COUNT OCCURENCES OF CHAR IN STRING  ---------------*/

int countOccurrencesOfChar(String s, char c) {
  int size = 0;
  for(int x = 0; x < s.length(); (s[x] == c) ? size++ : 0, x++);
  return size;
}

/*---------------  TURN AN UNTERMNATED CHAR ARRAY INTO A STRING  ---------------*/

String unterminated_strToStr(char c[], int len) {
    char s[len + 1];                                                            // create a character array one char longer then payload
    nullTermCharArray(c, len, s, len + 1);                                      // add a null char at the end of the payload array
    return String(s);                                                           // return s converted to a string
  }



/*---------------  NULL TERMINATE A CHAR ARRAY  ---------------*/

void nullTermCharArray(char sourceArr[], int sourceArrLen, char* newArr, int newArrLen) {
  for(int i = 0; i < sourceArrLen; i++) {
    newArr[i] = sourceArr[i];
  }
  newArr[newArrLen-1] = '\0';
}

/*---------------  PRINT CHAR VALUES OF CHAR ARRAY  ---------------*/

void printCharArrValues(char sourceArr[], int sourceArrLen) {
  for(int i = 0; i < sourceArrLen; i++) {
    Serial.print(sourceArr[i]);
    Serial.print("-");
  }
  Serial.println("");
}

/*---------------  NULL TERMINATE AN MQTT PAYLOAD  ---------------*/

String MQTT_payloadToStr(char* payload, size_t len) {
  char s[len + 1];                                                              // create a character array one char longer then payload
  nullTermCharArray(payload, len, s, len + 1);                                  // add a null char at the end of the payload array
  return String(s);                                                             // return s converted to a string
}

/*---------------  COMPARE TWO UNTERMINATED CHAR ARRAYS  ---------------*/

//#ifndef CMP_CHAR_ARRAY
int cmpchararrs(char arr1[], char arr2[], int len) {
  int i;
  for(i = 0; i < len; i++) {
    if(arr1[i] != arr2[i]) {
      return -1;
    }
  }
  return i;
}
//#endif

/*---------------    CONVERT BOOL ARRAY TO A STRING---------------*/

String boolArrayToString(bool a[], int len) {
  if (len <= 0 || a == nullptr) return  "-1";         // if this is not really an array they just return an empty string.
  int exactBytesNeeded = (len * 2) - 1;               // calculta the amount of memory the sting will need
  String s = "";                                      // creat the string
  s.reserve(exactBytesNeeded);                        // Allocates the perfect box size instantly!
  for (int i = 0; i < len; i++) {
    s += a[i] ? '1' : '0';
    if (i < len - 1) s += ',';
  }
  return s;
}
/*--------------- STRING TO CHAR ARRAY ---------------*/       // maybe should just return an empty buffer in case the buffer was only 1 byte in size???
// REPLACE WITH String(charArray, length) OR IS C++ string THEN String(charArray)
//#ifndef STR_TO_CHAR_ARRAY
void strToCharArray(char *buf, int bufLen, String str) {
  if (bufLen <= str.length()) {                                 // if the string length is greater than the buffer size
    buf[1] = '-';                                               // put -1 in the buffer
    buf[2] = '1';
  } else str.toCharArray(buf, bufLen);                          // else copy the string to the buffer
}
//#endif

/*--------------- COMPARE CHAR ARRAYS ---------------*/
// bool cmpCharArr(const char a[], const char b[], int len) {
// memcmp returns 0 if they are identical
//return memcmp(a, b, len) == 0;
//}
  bool cmpCharArr(char a[], char b[], int len) {
    for(int i = 0; i < len; i++) {
      if(a[i] != b[i]) return false;
    }
    return true;
  }

/*---------------    COUNT OCCURENCES OF CHAR IN A STRING    ---------------*/
/* REPLACE WITH
 #include <algorithm>               // Ensure this is at the top of your file (usually included by default)

 // 🚀 Count how many commas ',' are inside your string natively in one line:
 int count = std::count(myString.begin(), myString.end(), ',');

 */

int numOfCharInStr(String s, char c) {
  int count = 0;
  for(int i = 0; i < s.length(); i++) {
    if(s[i] == c) count++;
  }
  return count;
}

/*---------------  PAD A STRING---------------*/

String pad(String s, char padCh, int width)  {
  int len = s.length();
  if (len < width) {
    for (int i = 0; i < width - len; i++) {
      s = padCh + s;
    }
  }
  return s;
}

/*---------------  COUNT THE NUMBER OF OCCURRENCES OF A CHAR IN A STRING  ---------------*/

int countCharOccurrences(String s, char c) {
  int count = 0;
  for(int i = 0; i < s.length(); i++) {
    if(s[i] == c) count++;
  }
  return count;
}

/*---------------  CONVERT SOURCE ID TO NAME  ---------------*/

//String sourceIdtoName(source id) {
String srcIdToNm(source id) {
  switch(id) {
    case NOT_YET_SET: return "Not yet set";
    case WEB: return "Web page";
    case SWITCH: return "Switch";
    case NODERED: return "Node-Red";
    case REV_SHADE_DIR: return "Reverse shade direction";
    case _TIMER: return "Timer";
    case SW_AFTER_HOLDDOWN: return "Switch after hold down";
    case SECOND_PUSH: return "Second push";
    case PK_BK_RELEASE: return "Parking brake release";
    case SH_TO_FAR_DN_PKBK_REL: return "Shade to far down on parking brake release";
    default: mySP("ERROR: sourceIdtoName() - A bad ID was received. ID = " + String(id) + "\n", FN, LN);
  }
  return"ERROR: Unknown ID: " + String(id);
}

/*---------------  PRINT A CHAR N CAHRS WIDE OVER TIME  ---------------*/

void prntChar(PCHAR_PTR pcp) {
  if(pcp->count >= pcp->width) {
    Serial.println(pcp->c);
    pcp->count = 0;
  } else Serial.print(pcp->c);
  pcp->count++;
 }
