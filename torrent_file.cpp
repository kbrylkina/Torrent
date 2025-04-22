#include "torrent_file.h"
#include "byte_tools.h"
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <sstream>


TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream file_(filename);
    if (!file_.is_open()) {
        throw std::runtime_error("LoadTorrentFile Failed to open the file: " + filename);
    }
    std::stringstream s;
    std::string data_;
    s << file_.rdbuf();
    data_ = s.str();
    if (data_.empty()) {
        throw std::runtime_error("LoadTorrentFile File is empty or reading failed: " + filename);
    }
    TorrentFile torrent_;

    size_t cnt = 0, beg = 0, en = 0, begl = 0, enl = 0;
    std::string current = "", key = "", value = "", info_hash_str = "";
    for(size_t i = 1; i < data_.size() - 1; ++i) {
        if(cnt == 0 && current.empty()) {
            if(data_[i] <= '9' && data_[i] >= '0') {
                size_t ind = data_.find(':', i);
                cnt = std::stoll(data_.substr(i, ind - i));
                i = ind;
            }
            else if(data_[i] == 'i') {
                size_t ind = data_.find('e', i);
                current = data_.substr(i + 1, ind - i - 1);
                i = ind - 1;
            }
            else if(data_[i] == 'd') {
                beg = i;
                key.clear();
            }
            else if(data_[i] == 'e' && begl != 0 && begl > beg) {
                enl = 0;
                begl = 0;
            }
            else if(data_[i] == 'e' && en == 0 && beg != 0) {
                en = i;
            }
            else if(data_[i] == 'l') {
                begl = i;
            }
        }
        else if(cnt == 0 && !current.empty()) {
            if(key.empty()) {
                key = current;
            }
            else {
                value = current;
                if(key == "piece length") {
                    torrent_.pieceLength = std::stoll(value);
                }
                else if(key == "length") {
                    torrent_.length = std::stoll(value);
                }
                key.clear();
                value.clear();
            }
            current.clear();
        }
        else {
            current = data_.substr(i, cnt);
            i += cnt - 1;
            cnt = 0;
            if(key.empty()) {
                key = current;
            }
            else {
                if(begl != 0 && enl == 0) {
                    current.clear();
                    continue;
                }
                else if(begl != 0 && data_[i+1] == 'e') {
                    enl = i + 1;
                    value = data_.substr(begl, enl - begl + 1);
                }
                else {
                    value = current;
                }
                if(key == "announce") {
                    torrent_.announce = value;
                }
                else if(key == "comment") {
                    torrent_.comment = value;
                }
                else if(key == "piece length") {
                    torrent_.pieceLength = std::stoll(value);
                }
                else if(key == "length") {
                    torrent_.length = std::stoll(value);
                }
                else if(key == "pieces") {
                    size_t j;
                    for(j = 0; j + 20 < value.size(); j += 20) {
                        torrent_.pieceHashes.push_back((value.substr(j, 20)));
                    }
                    torrent_.pieceHashes.push_back((value.substr(j, value.size() - j)));
                }
                else if(key == "name") {
                    torrent_.name = value;
                }
                key.clear();
                value.clear();
            }
            current.clear();
        }
    }   
    
    torrent_.infoHash = CalculateSHA1(data_.substr(beg, en - beg + 1));
    return torrent_;
}
