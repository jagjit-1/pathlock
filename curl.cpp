#include <iostream>
#include <string>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>

using namespace std;

struct User {
	int age;
	string email;
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	((string *) userp)->append((char*) contents, size * nmemb);
	return size * nmemb;
};

bool performCurlRequest(const string& url, string& response) {
	CURL *curl = curl_easy_init();
	if (!curl) {
		cerr << "Failed to initialize CURL" << endl;
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
		return false;
	}

	return true;
}

bool parseJsonResponse(const string& jsonResponse, Json::Value& parsedRoot) {
	Json::CharReaderBuilder builder;
	Json::CharReader *reader = builder.newCharReader();
	string errs;

	bool parsingSuccessful = reader->parse(jsonResponse.c_str(), jsonResponse.c_str() + jsonResponse.size(), &parsedRoot, &errs);
	delete reader;

	if (!parsingSuccessful) {
		cerr << "Failed to parse JSON: " << errs << endl;
		return false;
	}

	return true;
}

int main() {
	map<string, User> cache;
	string api_url = "https://randomuser.me/api/?results=5";
	string response = "";
	curl_global_init(CURL_GLOBAL_DEFAULT);

	if (performCurlRequest(api_url, response)) {
		Json::Value root;
		if (parseJsonResponse(response, root)) {
			// do what you want to do with the response
			Json::StreamWriterBuilder builder;
			const Json::Value results = root["results"];
			for (const Json::Value &user : results) {
				if (user.isMember("login") && user.isMember("dob") && user.isMember("email")) {
						User newUser;
						string email = Json::writeString(builder, user["email"]);
						string username = Json::writeString(builder, user["login"]["username"]);
						int age = user["dob"]["age"].asInt();
						newUser.email = email;
						newUser.age = age;
						cache[username] = newUser;
				}
			}
		}
	}

	for (auto& it : cache) {
		cout << "stored username: " <<  it.first << " age: " << it.second.age << '\n';
	}

	curl_global_cleanup();
	return 0;
}
