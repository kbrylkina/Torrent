#pragma once

#include "peer.h"
#include "torrent_file.h"
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <cpr/cpr.h>
#include <iostream>

class TorrentTracker {
public:
    /*
     * url - адрес трекера, берется из поля announce в .torrent-файле
     */
    TorrentTracker(const std::string& url) : url_(url) {}

    /*
     * Получить список пиров у трекера и сохранить его для дальнейшей работы.
     * Запрос пиров происходит посредством HTTP GET запроса, данные передаются в формате bencode.
     * Такой же формат использовался в .torrent файле.
     *
     * tf: структура с разобранными данными из .torrent файла.
     * peerId: id, под которым представляется наш клиент.
     * port: порт, на котором наш клиент будет слушать входящие соединения (пока что мы не слушаем и на этот порт никто
     *  не сможет подключиться).
     */
    void UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
        cpr::Response res = cpr::Get(
            cpr::Url{tf.announce},
            cpr::Parameters {
                    {"info_hash", tf.infoHash},
                    {"peer_id", peerId},
                    {"port", std::to_string(port)},
                    {"uploaded", std::to_string(0)},
                    {"downloaded", std::to_string(0)},
                    {"left", std::to_string(tf.length)},
                    {"compact", std::to_string(1)}
                },
                cpr::Timeout{20000}
        );

        size_t cnt = 0, beg = 0, en = 0, begl = 0, enl = 0;
        std::vector<std::string> piece_peers;
        std::string current = "", key = "", value = "";
        for(size_t i = 1; i < res.text.size() - 1; ++i) {
            if(cnt == 0 && current.empty()) {
                if(res.text[i] <= '9' && res.text[i] >= '0') {
                    size_t ind = res.text.find(':', i);
                    cnt = std::stoll(res.text.substr(i, ind - i));
                    i = ind;
                }
                else if(res.text[i] == 'i') {
                    size_t ind = res.text.find('e', i);
                    current = res.text.substr(i + 1, ind - i - 1);
                    i = ind - 1;
                }
                else if(res.text[i] == 'd') {
                    beg = i;
                    key.clear();
                }
                else if(res.text[i] == 'e' && begl != 0 && begl > beg) {
                    enl = 0;
                    begl = 0;
                }
                else if(res.text[i] == 'e' && en == 0 && beg != 0) {
                    en = i;
                }
                else if(res.text[i] == 'l') {
                    begl = i;
                }
            }
            else if(cnt == 0 && !current.empty()) {
                if(key.empty()) {
                    key = current;
                }
                else {
                    value = current;
                    key.clear();
                    value.clear();
                }
                current.clear();
            }
            else {
                current = res.text.substr(i, cnt);
                i += cnt - 1;
                cnt = 0;
                if(key.empty()) {
                    key = current;
                }
                else {
                    value = current;
                    if(key == "peers") {
                        size_t j;
                        for(j = 0; j + 6 < value.size(); j += 6) {
                            piece_peers.push_back(value.substr(j, 6));
                        }
                        piece_peers.push_back(value.substr(j, value.size() - j));
                    }
                    key.clear();
                    value.clear();
                }
                current.clear();
            }
        }   

        for(auto& one_piece : piece_peers) {
            Peer peer;
            peer.ip = std::to_string((unsigned char)one_piece[0]);
            peer.ip += ".";
            peer.ip += std::to_string((unsigned char)one_piece[1]);
            peer.ip += ".";
            peer.ip += std::to_string((unsigned char)one_piece[2]);
            peer.ip += ".";
            peer.ip += std::to_string((unsigned char)one_piece[3]);
            peer.port = (((unsigned char)one_piece[4]) << 8) | ((unsigned char)one_piece[5]);
            peers_.push_back(peer);
        }
    }

    /*
     * Отдает полученный ранее список пиров
     */
    const std::vector<Peer>& GetPeers() const {
        return peers_;
    }

private:
    std::string url_;
    std::vector<Peer> peers_;
};

TorrentFile LoadTorrentFile(const std::string& filename) {
    std::ifstream file_(filename);
    std::stringstream s;
    std::string data_;
    s << file_.rdbuf();
    data_ = s.str();
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
                        torrent_.pieceHashes.push_back(value.substr(j, 20));
                    }
                    torrent_.pieceHashes.push_back(value.substr(j, value.size() - j));
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
    
    info_hash_str = data_.substr(beg, en - beg + 1);
    unsigned char hashes[20];
    SHA1(reinterpret_cast<const unsigned char*>(info_hash_str.c_str()), info_hash_str.size(), hashes);
    std::string str(reinterpret_cast<char*>(hashes), 20);
    torrent_.infoHash = str;
    return torrent_;
}