#include <iostream>
#include <queue>
#include <unordered_set>   
#include <vector>
#include <string>
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <sstream>
#include <iomanip>
#include <fstream>

using namespace std;

const string APIbase = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

// function to store API response
size_t Writecallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

// function to URL encode a string
string urlEncode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << setw(2) << int((unsigned char) c);
        }
    }

    return escaped.str();
}

// function to fetch neighbors API
vector<string> fetchNeighbors(const string& node) {
    vector<string> neighbors; // store neighbors
    CURL* curl = curl_easy_init(); // initialize curl
    if (!curl) {
        cerr << "Error with libcurl initialization" << endl;
        return neighbors;
    }

    string url = APIbase + urlEncode(node); // create URL
    string response_string;

    // set curl options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // set URL
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Writecallback); //set callback function
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string); // set response string
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // follow redirects

    // perform curl request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        cerr << "Error with curl request: " << curl_easy_strerror(res) << endl;
        curl_easy_cleanup(curl); // cleanup curl 
        return neighbors;
    }

    curl_easy_cleanup(curl); // cleanup curl

    // parse JSON response
    rapidjson::Document doc;
    doc.Parse(response_string.c_str());
    if (doc.HasMember("neighbors")) { // check if neighbors key exists
        const rapidjson::Value& neighbors_json = doc["neighbors"]; // get neighbors
        for (rapidjson::SizeType i = 0; i < neighbors_json.Size(); i++) {
            neighbors.push_back(neighbors_json[i].GetString()); // store neighbors
        }
    }

    return neighbors;
}

// BFS traversal
void bfs_traversal(const string& start_node, int depth, ofstream& outfile) {
    queue<pair<string, int>> q; // store node and depth
    unordered_set<string> visited; // store visited nodes
    q.push({start_node, 0}); // push start node
    visited.insert(start_node); // mark start node as visited
    while (!q.empty()) {
        auto [current_node, current_depth] = q.front(); // get current node and depth
        q.pop(); // remove the current node from the queue
        vector<string> neighbors = fetchNeighbors(current_node); // fetch neighbors
        outfile << "Node: " << current_node << ", Depth: " << current_depth << ", Neighbors: "; // write to output file
        for (const string& neighbor : neighbors) {
            outfile << neighbor << " ";
            if (visited.find(neighbor) == visited.end() && current_depth + 1 <= depth) { // check if neighbor is not visited and within depth
                q.push({neighbor, current_depth + 1}); // push neighbor and increment depth
                visited.insert(neighbor); // mark neighbor as visited
            }
        }
        outfile << endl;
    }
}

// main function
int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <start_node> <depth>" << endl;
        return 1;
    }
    string start_node = argv[1];
    int depth = stoi(argv[2]);
    cout << "Starting BFS traversal from node " << start_node << " at depth " << depth << endl;

    ofstream outfile("output.txt");
    if (!outfile.is_open()) {
        cerr << "Error opening output file" << endl;
        return 1;
    }

    bfs_traversal(start_node, depth, outfile);
    outfile.close();

    cout << "Traversal complete. Output written to output.txt" << endl;
    return 0;
}