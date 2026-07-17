#ifndef _MY_TEXT_H_
#define _MY_TEXT_H_


#define TO_MANY_OCCURRENCES "Counting "

//FUNCTION DEFINITIONS
extern bool strToBool(String s);
extern int strToArray(const String &s, char c, int limit, String arr[]);
extern String strArrToStr(String arr[], int len);
extern String uint8ArrToStr(uint8_t a[], int len, String seperator);
extern void strToUINT_8Arr(String s, char c, uint8_t arr[], int len);
extern int countOccurrencesOfChar(String s, char c);
extern String unterminated_strToStr(char c[], int len);
extern void nullTermCharArray(char sourceArr[], int sourceArrLen, char* newArr, int newArrLen);
extern void printCharArrValues(char sourceArr[], int sourceArrLen);
extern String MQTT_payloadToStr(char* payload, size_t len);
extern int cmpchararrs(char arr1[], char arr2[], int len);
String boolArrayToString(bool a[], int len);
extern void strToCharArray(char *buf, int bufLen, String str);
extern bool cmpCharArr(char a[], char b[], int len);
extern int numOfCharInStr(String s, char c);
extern String pad(String s, char padCh, int width);
extern int countCharOccurrences(String s, char c);
extern String srcIdToNm(source id);
extern void prntChar(PCHAR_PTR pcp);


#endif /* _MY_TEXT_H_ */
