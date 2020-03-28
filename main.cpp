/**************************************************************************
 * Covid-19 Data Monitor
 * 
 * Code to graphically display the current data of the Covid-19 pandemic
 * on a M5Stack (ESP32 MCU with integrated LCD)
 * 
 * Hague Nusseck @ electricidea
 * v1.6 28.March.2020
 * https://github.com/electricidea/M5Stack-Covid19-Monitor
 * 
 * Changelog:
 * v1.3 = - first published version
 * v1.4 = - Bugfix Screen height in graph routine
 *        - Changed color order
 * v1.5 = - Store data of multiple countries (30)
 *        - Load data after WiFi connection
 *        - Included weekly grid lines in graph
 *        - The entries are now editable
 *        - Bugfix scaling x-axis in Graph
 * v1.6 = - Added shifted graph analysis
 * 
 * Distributed as-is; no warranty is given.
**************************************************************************/

#include <Arduino.h>

#include <Preferences.h>
Preferences preferences;
String pref_fields[] = {"country_1", "country_2", "country_3", "country_4", "country_5"};

#include <M5Stack.h>
// install the library:
// pio lib install "M5Stack"

// Free Fonts for nice looking fonts on the screen
#include "Free_Fonts.h"

#include "WiFi.h"
#include <WiFiClientSecure.h>
// Init the Secure client object
WiFiClientSecure client;

// Stuff for the Graphical output
// The M5Stack screen pixel is 320x240, with the top left corner of the screen as the origin (0,0)
#define SCREEN_WIDTH 319
#define SCREEN_HEIGHT 239

// overall maximum to calculate the percentage of processing
#define max_number_countries 180
// country sekection array
// The names must match with the country names inside the JSON file
// 5 countries can be displayed at the same time
// The first one should always be index 0 = "All countries"
int country_selection[6]; 
// colors for the countries
const uint32_t country_color[6] = {LIGHTGREY, RED, GREEN, WHITE, MAGENTA, 0x51D};

// A String array to hold a set of country names
// Rules:
// The first entry is always for "All countries"
// The names must match with the country names inside the JSON file
#define n_countries 30
String country_names[n_countries] = {"All countries", "Australia", "Austria", "Belgium", "Brazil", "Canada", "China", 
                      "Croatia", "Finland", "France", "Germany", "Greece", "Iran", "Italy", "Japan", "Kosovo", "Mexico", 
                      "Netherlands", "North Macedonia", "Norway", "Poland", "Portugal", "Romania", "Russia", "Spain", 
                      "Sweden", "Switzerland", "Taiwan", "Turkey", "US"};

// Array field for the collected data out of the JSON file
// collected_data[confirmed, deaths][country][data point]
// the array is filled with data from all countries out of
// the country_names list
int collected_data[2][n_countries][SCREEN_WIDTH]; 
String data_name[] = {"confirmed", "deaths"};
// array to count the found data for each country
int data_count[n_countries];
// String to hold the last date found in the JSON file
std::string last_date = "";
// variable to switch between the graphical views
//  0 = no graphical display
// >0 = graphical and text display
int display_state = 0;
// varaible to switch between the menu states
int menu_state = 0;
// Index if a Country name field should be edited
int field_edit_index = 0;

// buffer for formatting numbers with thousand separator
// see function description for further  details
char format_buffer[11];
char thousand_separator = '.';

// WiFi network configuration:
// A simple method to configure multiple WiFi Access Configurations:
// Add the SSID and the password to the list.
// IMPORTANT: keep both arrays with the same length!
String WIFI_ssid[]     = {"Home_ssid", "Work_ssid", "Mobile_ssid", "Best-Friend_ssid"};
String WIFI_password[] = {"Home_pwd",  "Work_pwd",  "Mobile_pwd",  "Best-Friend_pwd"};

// the actual data are provided on this github page:
// https://github.com/pomber/covid19
// JSON time-series of coronavirus cases (confirmed, deaths and recovered) per country
// https://pomber.github.io/covid19/timeseries.json
const char*  data_server_name= "pomber.github.io";  // Server URL

// Certificate for Website:
// https://pomber.github.io
const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
"+OkuE6N36B9K\n" \
"-----END CERTIFICATE-----\n";



//==============================================================
// function forward declaration
void scan_WIFI();
boolean connect_Wifi(const char * ssid, const char * password);
const char *formatNumber(int value, char *buffer, int len);
void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace);
void Clear_Screen();
int process_data();
void display_data_graph(int data_select);
void display_data_graph_shifted(int data_select);
void display_data_text(int data_select);

//==============================================================
// Print the list of countries that are selected to show.
void print_list(int highlighted){
  Clear_Screen();
  // center aligned String output
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.setFreeFont(FF2);
  M5.Lcd.drawString("Covid-19 Monitor", (int)(M5.Lcd.width()/2), (int)(M5.Lcd.height()/6), 1);
  M5.Lcd.setFreeFont(FF1);
  // Country names
  int y_pos = (int)(M5.Lcd.height()/6)+50;
  for(int n=1; n<6; n++){
    M5.Lcd.setTextColor(country_color[n]);
    // highlight a line if selected
    if(highlighted == n){
      M5.Lcd.fillRect(0, y_pos-10, M5.Lcd.width(), 22, 0x528A);
    }
    M5.Lcd.drawString(country_names[country_selection[n]].c_str(), (int)(M5.Lcd.width()/2), y_pos, 1);
    y_pos = y_pos + 20;
  }
  // left aligned String output
  M5.Lcd.setTextDatum(TL_DATUM);
  M5.Lcd.setTextColor(WHITE);
}

//==============================================================
// Print a small menu at the bottom of the display above the buttons
void print_menu(int menu_index){
    M5.Lcd.fillRect(0, M5.Lcd.height()-25, M5.Lcd.width(), 25, 0x7BEF);
    M5.Lcd.setCursor(0, 230);    
    M5.Lcd.setFreeFont(FF1);
    M5.Lcd.setTextColor(WHITE);
    switch (menu_index) {
      case 0: { // never used 
        M5.Lcd.print("      -       -        - ");
        break;       
      }
      case 1: { // start menu
        M5.Lcd.print("    EDIT             SHOW");
        break;
      }
      case 2: { // Edit submenu for line selection
        M5.Lcd.print("    NEXT    EDIT     DONE ");
        break;
      }
      case 3: { // submenu for changing the country  
        M5.Lcd.print("      <       OK       > ");
        break;
      }
      case 4: { // menu for display the data 
        M5.Lcd.print("      <      BACK      > ");
        break;
      }
      default: { // should never been called
        M5.Lcd.print("      -       -        - ");
        break;
      }
    }
}

void setup() {
    // initialize the M5Stack object
    M5.begin();
    // configure the Lcd display
    M5.Lcd.setBrightness(10); //100 Brightness (0: Off - 255: Full)
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(1);
    Clear_Screen();
    // configure centered String output
    M5.Lcd.setTextDatum(CC_DATUM);
    M5.Lcd.setFreeFont(FF2);
    M5.Lcd.drawString("Covid-19 Monitor", (int)(M5.Lcd.width()/2), (int)(M5.Lcd.height()/2), 1);
    M5.Lcd.setFreeFont(FF1);
    M5.Lcd.drawString("Version 1.6 | 28.03.2020", (int)(M5.Lcd.width()/2), M5.Lcd.height()-20, 1);
    // wait 5 seconds before start file action
    delay(5000);
    // configure Top-Left oriented String output
    M5.Lcd.setTextDatum(TL_DATUM);
    // scan and display available WIFI networks
    Clear_Screen();
    scan_WIFI();
    delay(1000);
    // connect to WIFI
    // Try all access configurations until a connection could be established.
    int WIFI_location = 0;
    while(WiFi.status() != WL_CONNECTED){
      delay(1000);
      Clear_Screen();
      connect_Wifi(WIFI_ssid[WIFI_location].c_str(), WIFI_password[WIFI_location].c_str());
      WIFI_location++;
      if(WIFI_location >= (sizeof(WIFI_ssid)/sizeof(WIFI_ssid[0])))
        WIFI_location = 0;
    }
    M5.Lcd.println("");
    M5.Lcd.println("[OK] Connected to WiFi");
    delay(2000);
    // Download and parse the JSON data file
    Clear_Screen();
    // set the certificate for the https connection to github.io
    M5.Lcd.println("[OK] set certificate");
    client.setCACert(root_ca);
    // connect to the server
    M5.Lcd.println("Starting connection...");
    if (!client.connect(data_server_name, 443)){
      M5.Lcd.println("[ERR] Connection failed!");
      while(true)
        delay(100);
    } else {
      M5.Lcd.println("[OK] Connected to server");
      delay(200);
      // Make a HTTP request:
      client.println("GET https://pomber.github.io/covid19/timeseries.json HTTP/1.0");
      client.println("Host: pomber.github.io");
      client.println("Connection: close");
      client.println();
      // get the JSON data from the github server
      // and calculate the values
      process_data();
      // close the connection to the server
      client.stop();
    }
    // get the list of countries to be shown
    // the values are stored in the FLASH
    preferences.begin("country-config", false);
    country_selection[0] = 0; // Always "All countries"
    char Key_String[12];
    for(int n=1; n<6; n++){
      pref_fields[n].toCharArray(Key_String, sizeof(Key_String));
      country_selection[n] = preferences.getUInt(Key_String,n);
    }
    // close the preferences
    preferences.end();
    // ready to edit the list or visualize the data
    // print the start Menu
    field_edit_index = 0;
    print_list(field_edit_index);
    display_state = 0;
    menu_state = 1;
    print_menu(menu_state);
}


void loop() {
  M5.update();  
  // left Button
  if (M5.BtnA.wasPressed()){
    switch (menu_state) {
      case 1: {   // EDIT 
        menu_state = 2;
        field_edit_index = 1;
        print_list(field_edit_index);
        print_menu(menu_state);
        break;       
      }
      case 2: {   //  NEXT
        if(++field_edit_index > 5)
            field_edit_index = 1;
        print_list(field_edit_index);
        print_menu(menu_state);
        break;       
      }
      case 3: {   //  < (Country name selection)
        if(--country_selection[field_edit_index] == 0)
          country_selection[field_edit_index] = n_countries-1;
        print_list(field_edit_index);
        print_menu(menu_state);
        break;       
      }
      case 4: {   // Prev Page
        display_state--;
        if(display_state < 1)
          display_state = 10;  
        if(display_state < 3)
          display_data_graph(display_state);
        else{
          if(display_state < 5)
            display_data_graph_shifted(display_state);
          else
            display_data_text(display_state);
        }
        break;       
      }
    }
  }

  // center Button
  if (M5.BtnB.wasPressed()){
    switch (menu_state) {
      case 1: {   //  
      
        break;       
      }
      case 2: {   //  EDIT
        menu_state = 3;
        print_list(field_edit_index);
        print_menu(menu_state);
        break;       
      }
      case 3: {   //  OK
        menu_state = 2;
        char Key_String[12];
        pref_fields[field_edit_index].toCharArray(Key_String, sizeof(Key_String));
        // save selection
        preferences.begin("country-config", false);
        preferences.putUInt(Key_String, country_selection[field_edit_index]);
        preferences.end();
        print_list(field_edit_index);
        print_menu(menu_state);
        break;       
      }
      case 4: {   // Back 
        // print the start Menu
        display_state = 0;
        menu_state = 1;
        print_list(0);
        print_menu(menu_state);
        break;       
      }
    }
  }

  // right Button
  if (M5.BtnC.wasPressed()){
    switch (menu_state) {
      case 1: {   // SHOW 
        menu_state = 4;
        display_state = 1; 
        display_data_graph(display_state);
        break;       
      }
      case 2: {   // Done 
        // print the start Menu
        field_edit_index = 0;
        display_state = 0;
        menu_state = 1;
        print_list(0);
        print_menu(menu_state);
        break;       
      }
      case 3: {   //  > (Country name selection)
        if(++country_selection[field_edit_index] == n_countries)
          country_selection[field_edit_index] = 1;
        print_list(field_edit_index);
        print_menu(menu_state);
        break;       
      }
      case 4: {   // Next Page 
        display_state++;
        if(display_state > 10)
          display_state = 1;  
        if(display_state < 3)
          display_data_graph(display_state);
        else{
          if(display_state < 5)
            display_data_graph_shifted(display_state);
          else
            display_data_text(display_state);
        }
        break;       
      }
    }
  }
}



//==============================================================
// Scan for available Wifi networks
// print result als simple list
void scan_WIFI() {
      M5.Lcd.println("WiFi scan ...");
      // WiFi.scanNetworks returns the number of networks found
      int n = WiFi.scanNetworks();
      if (n == 0) {
          M5.Lcd.println("[ERR] no networks found");
      } else {
          //M5.Lcd.println("[OK] %i networks found:\n",n);
          for (int i = 0; i < n; ++i) {
              // Print SSID for each network found
              M5.Lcd.printf("  %i: ",i+1);
              M5.Lcd.println(WiFi.SSID(i));
              delay(10);
          }
      }
}

//==============================================================
// establish the connection to an Wifi Access point
boolean connect_Wifi(const char * ssid, const char * password){
  // Establish connection to the specified network until success.
  // Important to disconnect in case that there is a valid connection
  WiFi.disconnect();
  M5.Lcd.println("Connecting to ");
  M5.Lcd.println(ssid);
  delay(500);
  //Start connecting (done by the ESP in the background)
  WiFi.begin(ssid, password);
  // read wifi Status
  wl_status_t wifi_Status = WiFi.status();  
  int n_trials = 0;
  // loop while waiting for Wifi connection
  // run only for 5 trials.
  while (wifi_Status != WL_CONNECTED && n_trials < 5) {
    // Check periodicaly the connection status using WiFi.status()
    // Keep checking until ESP has successfuly connected
    // or maximum number of trials is reached
    wifi_Status = WiFi.status();
    n_trials++;
    switch(wifi_Status){
      case WL_NO_SSID_AVAIL:
          M5.Lcd.println("[ERR] SSID not available");
          break;
      case WL_CONNECT_FAILED:
          M5.Lcd.println("[ERR] Connection failed");
          break;
      case WL_CONNECTION_LOST:
          M5.Lcd.println("[ERR] Connection lost");
          break;
      case WL_DISCONNECTED:
          M5.Lcd.println("[ERR] WiFi disconnected");
          break;
      case WL_IDLE_STATUS:
          M5.Lcd.println("[ERR] WiFi idle status");
          break;
      case WL_SCAN_COMPLETED:
          M5.Lcd.println("[OK] WiFi scan completed");
          break;
      case WL_CONNECTED:
          M5.Lcd.println("[OK] WiFi connected");
          break;
      default:
          M5.Lcd.println("[ERR] unknown Status");
          break;
    }
    delay(500);
  }
  if(wifi_Status == WL_CONNECTED){
    // connected
    M5.Lcd.print("IP address: ");
    M5.Lcd.println(WiFi.localIP());
    return true;
  } else {
    // not connected
    M5.Lcd.println("");
    M5.Lcd.println("[ERR] unable to connect Wifi");
    return false;
  }
}


//==============================================================
// Function to format a integer number into a string 
// with thousand separator and a fixed length
// useful to get a nice right aligned string output
// inspired from:
// https://stackoverflow.com/questions/1449805/how-to-format-a-number-from-1123456789-to-1-123-456-789-in-c
// Use as following:
//
// char buffer[11]; // 10 char + \0 = 11
// M5.Lcd.printf("%s\n", formatNumber(1234567, format_buffer, sizeof(buffer)));
// output: " 1.234.567"
const char *formatNumber (int value, char *buffer, int len) {
    int charCount;
    char *endOfbuffer = buffer+len;
    int savedValue = value;
    if (value < 0)
        value = - value;
    // \0 Termination char at the end
    *--endOfbuffer = 0;
    charCount = -1;
    do{
        if (++charCount == 3){
            charCount = 0;
            *--endOfbuffer = thousand_separator;
            if(endOfbuffer <= buffer)
              break;
        }
        *--endOfbuffer = (char) (value % 10 + '0');
    } while ((value /= 10) != 0 && endOfbuffer > buffer);
    // add a minus sign if the original value was negative
    if (savedValue < 0 && endOfbuffer > buffer)
        *--endOfbuffer = '-';
    // fill up with spaces, to the full string length
    while(endOfbuffer > buffer){
      *--endOfbuffer = ' ';
    }
    return endOfbuffer;
}

//==============================================================
// A String-Replace function...
// from:
// https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
void ReplaceStringInPlace(std::string& subject, const std::string& search,
                          const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
         subject.replace(pos, search.length(), replace);
         pos += replace.length();
    }
}

//==============================================================
// Clear the entire screen and add one row
// The added row is important. Otherwise the first row is not visible
void Clear_Screen(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("");
}

//==============================================================
// receive the JSON file from the server
// go through the file line by line
// search for keywords and sum the values
int process_data(){
  // Number of lines received from the server and processed
  int line_count = 0;
  // total number of countries found in the JSON file
  int countries_found = 0;
  for(int n=0; n<6; n++)
    data_count[n] = 0;
  // strings to hold the received data
  std::string rcv_line = "";
  std::string analyze_line = "";
  // skip the header data
  while (client.connected()) {
    rcv_line = client.readStringUntil('\n').c_str();
    if (rcv_line == "\r") {
      M5.Lcd.println("[OK] header received");
      break;
    }
  }
  // Index of a found country section
  // Country_index = 0 --> looking for a configured countrie
  // Country_index > 0 --> country found. collecting data
  int Country_index = 0;

  // Basic format information from the JSOM data:
  // Country START string:
  // "  \"Germany\": [\n"
  // Country END string:
  // "  ],\n" or "  ]\n"
  // value strings:
  // "      \"confirmed\": 0,\n"
  // "      \"deaths\": 0,\n"
  // "      \"recovered\": 0\n"
  
  // if there are incoming bytes available
  while (client.available()) {
    // get one line from the server data
    rcv_line = client.readStringUntil('\n').c_str();
    // looking for the "country seperator"
    if (rcv_line.find(": [", 0)  != std::string::npos){ 
      // count the overall number of countries
      countries_found++;
      // the data from all countries should summed up
      data_count[0] = 0;
      // print the percentage of processing
      Clear_Screen();
      M5.Lcd.setTextDatum(CC_DATUM);
      M5.Lcd.setFreeFont(FF3);
      char text_buffer[32];
      snprintf(text_buffer, sizeof(text_buffer), "%3.0f%%",
              (100.0/max_number_countries)*countries_found);
      M5.Lcd.drawString(text_buffer, (int)(M5.Lcd.width()/2), (int)(M5.Lcd.height()/2), 1);
    }
    // count the number of confirmed, deaths and recovered over all countries
    // looking for a "confirmed" data line
    if (rcv_line.find("confirmed", 0)  != std::string::npos){
      analyze_line = rcv_line;
      // delete everything out of the string exept the number
      ReplaceStringInPlace(analyze_line, "      \"confirmed\": ", "");
      ReplaceStringInPlace(analyze_line, ",", "");
      ReplaceStringInPlace(analyze_line, "\n", "");
      int value = atoi(analyze_line.c_str());
      // init the value for "all countries" (index 0) with the values of the first country
      if(countries_found == 1){
        collected_data[0][0][data_count[0]] = value;
      } else {
        // Otherwise add the values of the other countries
        collected_data[0][0][data_count[0]] = collected_data[0][0][data_count[0]]+value;
      }
      // if we are inside an configure country section
      if(Country_index > 0){
        collected_data[0][Country_index][data_count[Country_index]]=value;
      }
    }
    // looking for a "deaths" data line
    if (rcv_line.find("deaths", 0)  != std::string::npos){
      analyze_line = rcv_line;
      // delete everything out of the string exept the number
      ReplaceStringInPlace(analyze_line, "      \"deaths\": ", "");
      ReplaceStringInPlace(analyze_line, ",", "");
      ReplaceStringInPlace(analyze_line, "\n", "");
      int value = atoi(analyze_line.c_str());
      // init the value for all countries with the values of the first country
      if(countries_found == 1){
        collected_data[1][0][data_count[0]] = value;
      } else {
        // Otherwise add the values of the other countries
        collected_data[1][0][data_count[0]] = collected_data[1][0][data_count[0]]+value;
      }
      data_count[0]++;
      // Array can contain maximum SCREEN_WIDTH datapoints
      if(data_count[0] >= SCREEN_WIDTH)
        data_count[0] = 0;
      // if we are inside an configure country section
      if(Country_index > 0){
        collected_data[1][Country_index][data_count[Country_index]]=value;
        data_count[Country_index]++;
        if(data_count[Country_index] >= SCREEN_WIDTH)
          data_count[Country_index] = 0;
      }
    }
    // store the last "date" line to get the last actualization date
    // looking for a "date" data line
    if (rcv_line.find("date", 0)  != std::string::npos){
      last_date = rcv_line;
    }
    // check for country out of the list,
    // if outside an country section
    if(Country_index == 0) {
      for(int n=1; n<n_countries; n++){
        if (rcv_line.find(country_names[n].c_str(), 0)  != std::string::npos){
          Country_index = n;
        }
      }
    } else {
      // otherwise check for END of country-section
      if (rcv_line.find("]", 0)  != std::string::npos){
        Country_index = 0;
      }
    }
    line_count++;
    // Sometimes, we need to wait for new data from the server
    // to not exit the loop before all data are received
    // Last data line is "}"
    for(int n=0; n<10; n++){
      if (!client.available() && rcv_line != "}"){
        delay(550);
      }
    }
  }
  // delete everything out of the last_date string exept the date
  ReplaceStringInPlace(last_date, "      \"date\": \"", "");
  ReplaceStringInPlace(last_date, "\",", "");
  ReplaceStringInPlace(last_date, "\n", "");
  return line_count;
}

//==============================================================
// display the data as curves / graphs including a legend
void display_data_graph(int data_select){
  print_menu(4);
  delay(200);
  M5.Lcd.fillScreen(BLACK);
  int selected_country;
  // get maximum value to scale the y-axis
  int max_y = 0;
  for(int n=1; n<6; n++){
    selected_country = country_selection[n];
    for(int i = 0; i < data_count[selected_country]; i++){
      if(collected_data[data_select-1][selected_country][i] > max_y)
        max_y = collected_data[data_select-1][selected_country][i];
    }
  }
  // draw weekly grid lines
  // start from the end and go backwards
  int xpos = data_count[country_selection[1]]-7;
  float x_scale = float(SCREEN_WIDTH) / data_count[1];
  while(xpos > 0){
    M5.Lcd.drawLine(trunc(x_scale*xpos), 0, trunc(x_scale*xpos), (SCREEN_HEIGHT-1), 0x528A);
    xpos = xpos -7;
  }
  // draw line graph
  for(int n=1; n<6; n++){
    selected_country = country_selection[n];
    x_scale = float(SCREEN_WIDTH) / data_count[selected_country];
    for(int i = 1; i < data_count[selected_country]; i++){
      M5.Lcd.drawLine(trunc(x_scale*(i-1)), 
                      (SCREEN_HEIGHT-1)-(round((float((SCREEN_HEIGHT-1)) / max_y) * collected_data[data_select-1][selected_country][i-1])), 
                      trunc(x_scale*i), 
                      (SCREEN_HEIGHT-1)-(round((float((SCREEN_HEIGHT-1)) / max_y) * collected_data[data_select-1][selected_country][i])), 
                      country_color[n]);
    }
  }
  // draw legend
  M5.Lcd.setFreeFont(FF1);
  M5.Lcd.setCursor(0, 0);
  // headline
  M5.Lcd.printf("\n%s (%s)\n\n", data_name[data_select-1], last_date.c_str());
  // Country name and value
  for(int n=1; n<6; n++){
    selected_country = country_selection[n];
    M5.Lcd.setTextColor(country_color[n]);
    M5.Lcd.printf("%s:\n%s\n", country_names[selected_country].c_str(), 
                               formatNumber(collected_data[data_select-1][selected_country][data_count[selected_country]-1], 
                                            format_buffer, 
                                            sizeof(format_buffer)));
  }
  M5.Lcd.setTextColor(WHITE);
}

//==============================================================
// display the data as curves shifted in x so that all countries
// aligned with the first increase of data
void display_data_graph_shifted(int data_select){
  print_menu(4);
  delay(200);
  M5.Lcd.fillScreen(BLACK);
  int selected_country;
  // get maximum value to scale the y-axis
  int max_y = 0;
  for(int n=1; n<6; n++){
    selected_country = country_selection[n];
    for(int i = 0; i < data_count[selected_country]; i++){
      if(collected_data[data_select-3][selected_country][i] > max_y)
        max_y = collected_data[data_select-3][selected_country][i];
    }
  }
  // draw weekly grid lines
  // start from the left and go forward
  int xpos = 0;
  float x_scale = float(SCREEN_WIDTH) / data_count[1];
  while(xpos < data_count[country_selection[1]]){
    M5.Lcd.drawLine(trunc(x_scale*xpos), 0, trunc(x_scale*xpos), (SCREEN_HEIGHT-1), 0x528A);
    xpos = xpos +7;
  }
  // find the x position of first grow of data for each country
  // threshold is a value above 2000 (confirmed) and 100 (deaths)
  int y_threshold = 2000;
  if(data_select-3 == 1)
    y_threshold = 100;
  int first_y[6];
  int first_y_max = 0;
  for(int n=1; n<6; n++){
    selected_country = country_selection[n];
    first_y[n] = 0;
    for(int i = 0; i < data_count[selected_country]; i++){
      if(collected_data[data_select-3][selected_country][i] < y_threshold){
        first_y[n] = i-14; // go back 2 Weeks
        // find the largest position
        if(i > first_y_max)
          first_y_max = i;
      }
    }
  }
  // draw the shifted line graph
  for(int n=1; n<6; n++){
    selected_country = country_selection[n];
    x_scale = float(SCREEN_WIDTH) / data_count[selected_country];
    for(int i = first_y[n]; i < data_count[selected_country]; i++){
      if(i > 0){
        M5.Lcd.drawLine(trunc(x_scale*((i-first_y[n])-1)), 
                        (SCREEN_HEIGHT-1)-(round((float((SCREEN_HEIGHT-1)) / max_y) * collected_data[data_select-3][selected_country][i-1])), 
                        trunc(x_scale*(i-first_y[n])), 
                        (SCREEN_HEIGHT-1)-(round((float((SCREEN_HEIGHT-1)) / max_y) * collected_data[data_select-3][selected_country][i])), 
                        country_color[n]);
      }
    }
  }
  // draw legend
  M5.Lcd.setFreeFont(FF1);
  M5.Lcd.setCursor(0, 0);
  // headline
  M5.Lcd.printf("\n%s (shifted)\n\n", data_name[data_select-3]);
  // Country name
  for(int n=1; n<6; n++){
    selected_country = country_selection[n];
    M5.Lcd.setTextColor(country_color[n]);
    M5.Lcd.printf("%s\n", country_names[selected_country].c_str());
  }
  M5.Lcd.setTextColor(WHITE);
}

//==============================================================
// display the data as summarized text output
void display_data_text(int data_select){
  Clear_Screen();
  print_menu(4);
  int selected_country = country_selection[data_select-5];
  // draw text output
  M5.Lcd.setFreeFont(FF2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(country_color[data_select-5]);
  M5.Lcd.printf("\n%s:\n\n", country_names[selected_country].c_str());
  M5.Lcd.setFreeFont(FF1);
  M5.Lcd.setTextColor(WHITE);
  int n_confirmed = collected_data[0][selected_country][data_count[selected_country]-1];
  int n_deaths = collected_data[1][selected_country][data_count[selected_country]-1];
  M5.Lcd.printf("  confirmed:  %s\n", formatNumber(n_confirmed, format_buffer, sizeof(format_buffer)));
  M5.Lcd.printf("  deaths:     %s\n\n",    formatNumber(n_deaths, format_buffer, sizeof(format_buffer)));
  M5.Lcd.printf("  death rate:    %6.2f%%\n", (100.0/n_confirmed) * n_deaths);
}