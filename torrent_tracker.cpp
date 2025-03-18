#include "torrent_tracker.h"
#include "byte_tools.h"
#include <cpr/cpr.h>

TorrentTracker::TorrentTracker(const std::string& url) : url_(url) {}

void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port) {
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

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}
