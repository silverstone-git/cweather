#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#define JSON_SIZE 3000
#define DDGRESPONSE 200000

size_t curl_chunk_data_saver(void *buffer, size_t byte_size, size_t no_of_bytes, char (*response_str)[]) {
	// a function to save response chunk each time it is run to a variable
	// response being passed as meta into this function every time new chunk rolls in
	size_t fullsize = byte_size * no_of_bytes;	// ---- example -> 0.5 byte * 30 would be 15 bytes in the chunk
	char cur_char;
	int i;

	// concatenating the buffer packet to the response_str, which points to response array in main
	strcat(*response_str, buffer);

	// checking for garbage values till first 100 chars, and replacing them with whitespace till it hits the {
	for(i = 0; i < 100; i++) {
		cur_char = (*response_str)[i];
		if(cur_char == '{')
			break;
		(*response_str)[i] = ' ';
	}

	return fullsize;
}


size_t curl_chunk_data_saver_nojson(void *buffer, size_t byte_size, size_t no_of_bytes, char (*response_str)[]) {
	// a function to save response chunk each time it is run to a variable
	// response being passed as meta into this function every time new chunk rolls in
	size_t fullsize = byte_size * no_of_bytes;
	char cur_char;
	int i;
	strcat(*response_str, buffer);
	return fullsize;
}


void urlsafe(char (*loc)[], int length) {
	// making the entered location argument free of weird characters and replacing them with space
	char newstring[100];
	int i;
	for(i = 0; i < length; i++) {
		if(((*loc)[i] < 48 || ((*loc)[i] > 57 && (*loc)[i] < 65) || ((*loc)[i] > 90 && (*loc)[i] < 97) || (*loc)[i] > 122) && (*loc)[i] != 44 && (*loc)[i] != 32)
			newstring[i] = ' ';
		else
			newstring[i] = (*loc)[i];
	}
	newstring[i] = '\0';

    i = 0;
    while(newstring[i] != '\0') {
        (*loc)[i] = newstring[i];
        i++;
    }
    
    (*loc)[i] = '\0';

}

void getcurrent(char (*loc)[]) {

    // automatically detect the location by scraping ddg

	char urltemp[300];
	char response[DDGRESPONSE] = "";

	strncpy(urltemp, "https://lite.duckduckgo.com/lite/?q=my+ip", 300);
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, urltemp);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_chunk_data_saver_nojson);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	curl_easy_cleanup(curl);


	// searching for the required part in response by calling strstr and then subtracting addresses
    // (since we know addresses in an array are stored linearly)
	char substr[] = "&iar=maps_maps";
	char *mainstring = response;
	char *result = strstr(mainstring, substr);
	int position = result - mainstring;

	// the position variable is the starting index of the substring in the mainstring, add the length of substring and you get the end index
	position += strlen(substr);

	// there is an apostrophe to close the string link in duckduckgo and also a tag close '>' with it, that makes 2
	position += 2;

    // copying the response from position to when the address ends and pincode starts in parenthesis
	int i = 0;
	char cur_char;
	while(5>4) {
		cur_char = response[position];
		if(cur_char  == '(')
			break;
		(*loc)[i] = cur_char;
		position ++;
		i++;
	}

    (*loc)[i] = '\0';

	response[0] = '\0';
}


void geolocate(char loc[], int size_of_loc, char (*lat)[], char (*lon)[]) {
	int urlified_i = 0;
	int i;
	CURL *curl;
	CURLcode res;
	char urlified_loc[100];
	char urltemp[300];
	char response[JSON_SIZE] = "";
    char tobesafed[100];

    strcpy(tobesafed, loc);

	urlsafe(&tobesafed, size_of_loc);

	for(i = 0; i < size_of_loc; i ++) {
		if(tobesafed[i] == 32) {
			// if the location string in the command line argument is a white space, convert it into
			// %20 every time
			urlified_loc[urlified_i++] = '%';
			urlified_loc[urlified_i++] = '2';
			urlified_loc[urlified_i++] = '0';
		} else {
			urlified_loc[urlified_i++] = tobesafed[i];
		}
	}

	urlified_loc[urlified_i] = '\0';
	sprintf(urltemp, "https://nominatim.openstreetmap.org/search/%s?format=json&addressdetails=1&limit=1", urlified_loc);
	curl = curl_easy_init();

	if(curl) {

		// setting the options for curl - the url to open, the writer function to write the response json string
		// and a response string to update in the function as a pointer, and a user agent, for server to trust the client
		curl_easy_setopt(curl, CURLOPT_URL, urltemp);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_chunk_data_saver);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
		res = curl_easy_perform(curl);

		// handling the curl response error code
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

	} else {
		printf("curl could not initialize\n");
	}

	curl_easy_cleanup(curl);

    if((int)strlen(response) == 100) {
        printf("No hits for the entered location!\n");
        exit(1);
    }

	// parsing the json string
	cJSON *json;
    json = cJSON_Parse(response);
	response[0] = '\0';
    strcpy(*lat, cJSON_GetObjectItemCaseSensitive(json, "lat") -> valuestring);
    strcpy(*lon, cJSON_GetObjectItemCaseSensitive(json, "lon") -> valuestring);
}

int main(int argc, char* argv[]) {

	// initializing variables for curl object, read buffer, url template string, and the response string
	char first_arg[100];
	CURL *curl;
	CURLcode res;
	FILE *fptr;
	char apikey[100];
	char lat[20];
	char lon[20];
	char loc[100];
	int is_loc = 0;
	char urltemp[300];
	char response[JSON_SIZE] = "";
	int length_of_loc;
    int i;

	// reading the api key from the folder
        fptr = fopen("owmapi.key", "r");
        fgets(apikey, 100, fptr);
        fclose(fptr);

	// removing newline from api key to avoid curl format error
	apikey[strcspn(apikey, "\n")] = 0;

	// handling the command line argument not given case
	if(argc < 2) {
		// get location by fetching duckduckgo page
		is_loc = 1;
		getcurrent(&loc);
		length_of_loc = (int) strlen(loc);
		urlsafe(&loc, length_of_loc);

	} else if(argc == 2) {
		// if the location argument is given, prepare to search it in nominatim to get lat and lon
		is_loc = 1;
		strcpy(loc, argv[1]);
		length_of_loc = (int) strlen(loc);

	} else {
		// if there are 3 args, ie, lat and lon given, directly take those and query it in on owm
		strcpy(lat, argv[1]);
		strcpy(lon, argv[2]);
	}

	if(is_loc == 1) {
		// in case the user entered location name instead of coordinates,
		// calling the geolocate function to search that place's coordinates
		geolocate(loc, length_of_loc, &lat, &lon);
	}

	//printf("At %s, %s\n", lat, lon);
	sprintf(urltemp, "https://api.openweathermap.org/data/2.5/weather?lat=%s&lon=%s&appid=%s", lat, lon, apikey);
	curl = curl_easy_init();

	if(curl) {
		// setting the options for curl - the url to open, the writer function to write the response json string
		// and a response string to update in the function as a pointer
		curl_easy_setopt(curl, CURLOPT_URL, urltemp);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_chunk_data_saver);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		res = curl_easy_perform(curl);

		// handling the curl response error code
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

	} else {
		printf("curl could not initialize\n");
	}

	curl_easy_cleanup(curl);


    // defining pointers for cjson to put values in when parsing
    cJSON *json;
    const cJSON *main_obj = NULL;
    const cJSON *temp = NULL;
    const cJSON *weather_array = NULL;
    const cJSON *weather = NULL;
    char descstr[20];
    // Parsing the JSON, storing stuff in variables
    json = cJSON_Parse(response);
    // deleting this string to make curl requests using same variable later
    response[0] = '\0';
    main_obj  = cJSON_GetObjectItemCaseSensitive(json, "main");
    temp  = cJSON_GetObjectItemCaseSensitive(main_obj, "temp");
    // traversing through the array of objects in the root object (it has only one element by default)
    weather_array = cJSON_GetObjectItemCaseSensitive(json, "weather");
    cJSON_ArrayForEach(weather, weather_array) {
        cJSON *desc = cJSON_GetObjectItemCaseSensitive(weather, "description");
        strcpy(descstr, (desc->valuestring));
    }

        // checking which weather icon to use
        char weather_icon[20];
        if(strcmp(descstr, "mist") == 0)
                strncpy(weather_icon, "🌬️", sizeof(weather_icon));
        else if(strcmp(descstr, "thunderstorm") == 0)
                strncpy(weather_icon, "🌩️", sizeof(weather_icon));
        else if(strcmp(descstr, "rain") == 0)
                strncpy(weather_icon, "☔", sizeof(weather_icon));
        else if(strcmp(descstr, "clear sky") == 0)
                strncpy(weather_icon, "☀️", sizeof(weather_icon));
        else if(strcmp(descstr, "snow") == 0)
                strncpy(weather_icon, "❄️", sizeof(weather_icon));
        else if(strcmp(descstr, "smoke") == 0)
                strncpy(weather_icon, "🚬", sizeof(weather_icon));
        else if(strcmp(descstr, "fog") == 0)
                strncpy(weather_icon, "🌁", sizeof(weather_icon));
        else if(strcmp(descstr, "light rain") == 0)
                strncpy(weather_icon, "🌈", sizeof(weather_icon));
        else if(strcmp(descstr, "moderate rain") == 0)
                strncpy(weather_icon, "🌧️", sizeof(weather_icon));
        else if(strcmp(descstr, "overcast clouds") == 0)
                strncpy(weather_icon, "🌫️", sizeof(weather_icon));
        else if(strcmp(descstr, "scattered clouds") == 0)
                strncpy(weather_icon, "🌫️", sizeof(weather_icon));
        else if(strcmp(descstr, "drizzle") == 0)
                strncpy(weather_icon, "💦", sizeof(weather_icon));
        else if(strcmp(descstr, "haze") == 0)
                strncpy(weather_icon, "🌫️", sizeof(weather_icon));
        else if(strcmp(descstr, "broken clouds") == 0)
                strncpy(weather_icon, "☁", sizeof(weather_icon));
        else if(strcmp(descstr, "few clouds") == 0)
                strncpy(weather_icon, "☁", sizeof(weather_icon));
        else if(strcmp(descstr, "tornado") == 0)
                strncpy(weather_icon, "🌪", sizeof(weather_icon));
        else
                strncpy(weather_icon, "🌥️", sizeof(weather_icon));

        //printf("%.2lf °C %s  %s\n", (temp->valuedouble)-273.15, descstr, weather_icon);
        printf("%.2lf K %s  %s\n", temp->valuedouble, descstr, weather_icon);


	return 0;
}
