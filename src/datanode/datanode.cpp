#include "../../include/datanode.h"
#include "../../include/json/json.h"
#include "../jsoncpp.cpp"
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>


Datanode::Datanode(std::string ip, int port)
    : ip_(ip), port_(port),
      acceptor_(io_context_,
                asio::ip::tcp::endpoint(
                    asio::ip::address::from_string(ip.c_str()), port)) {
  std::string url = "tcp://" + ip_ + ":" + std::to_string(port_ + 1000);
  redis_ = std::make_unique<sw::redis::Redis>(url);
  filename="/home/cxm/cacheproject/happylrc/jsonout/"+ ip_ + std::to_string(port_) +"data.json";
  file_.open(filename, std::ios::app | std::ios::in | std::ios::binary);
  cur_offset_ = 0;
}

Datanode::~Datanode() { acceptor_.close(); }

void Datanode::keep_working() {
  for (;;) {
    asio::ip::tcp::socket peer(io_context_);
    acceptor_.accept(peer);
    /*asio flag*/
    std::vector<unsigned char> flag_buf(sizeof(int));
    asio::read(peer, asio::buffer(flag_buf, flag_buf.size()));
    int flag = bytes_to_int(flag_buf);

    if (flag == 0) {
      std::vector<unsigned char> value_or_key_size_buf(sizeof(int));
      asio::read(peer, asio::buffer(value_or_key_size_buf,
                                    value_or_key_size_buf.size()));
      int key_size = bytes_to_int(value_or_key_size_buf);
      asio::read(peer, asio::buffer(value_or_key_size_buf,
                                    value_or_key_size_buf.size()));
      int value_size = bytes_to_int(value_or_key_size_buf);
      std::string key_buf(key_size, 0);
      std::string value_buf(value_size, 0);
      asio::read(peer, asio::buffer(key_buf.data(), key_buf.size()));
      asio::read(peer, asio::buffer(value_buf.data(), value_buf.size()));

      redis_->set(key_buf, value_buf);

      std::vector<char> finish(1);
      asio::write(peer, asio::buffer(finish, finish.size()));

      asio::error_code ignore_ec;
      peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
      peer.close(ignore_ec);
    } else if (flag ==1) {
      std::vector<unsigned char> key_size_buf(sizeof(int));
      asio::read(peer, asio::buffer(key_size_buf, key_size_buf.size()));
      int key_size = bytes_to_int(key_size_buf);

      std::string key_buf(key_size, 0);
      asio::read(peer, asio::buffer(key_buf.data(), key_buf.size()));

      auto value_returned = redis_->get(key_buf);
      my_assert(value_returned.has_value());
      std::string value = value_returned.value();

      asio::write(peer, asio::buffer(value.data(), value.length()));

      asio::error_code ignore_ec;
      peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
      peer.close(ignore_ec);
    } else if (flag ==2) {
      std::vector<unsigned char> value_or_key_size_buf(sizeof(int));
      asio::read(peer, asio::buffer(value_or_key_size_buf,
                                    value_or_key_size_buf.size()));
      int key_size = bytes_to_int(value_or_key_size_buf);
      asio::read(peer, asio::buffer(value_or_key_size_buf,
                                    value_or_key_size_buf.size()));
      int value_size = bytes_to_int(value_or_key_size_buf);
      std::string key_buf(key_size, 0);
      std::string value_buf(value_size, 0);
      asio::read(peer, asio::buffer(key_buf.data(), key_buf.size()));
      asio::read(peer, asio::buffer(value_buf.data(), value_buf.size()));

      write_key_value_to_disk(key_buf, value_buf);
      

      /*************************************************************************/
      std::vector<char> finish(1);
      asio::write(peer, asio::buffer(finish, finish.size()));

      asio::error_code ignore_ec;
      peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
      peer.close(ignore_ec);
    } else if (flag ==3) {/*读disk*/
      std::vector<unsigned char> key_size_buf(sizeof(int));
      asio::read(peer, asio::buffer(key_size_buf, key_size_buf.size()));
      int key_size = bytes_to_int(key_size_buf);

      std::string key_buf(key_size, 0);
      asio::read(peer, asio::buffer(key_buf.data(), key_buf.size()));

      std::string value = find_value_by_key_from_disk(key_buf);
      
      
      /*************************************************************************/
      asio::write(peer, asio::buffer(value.data(), value.length()));

      asio::error_code ignore_ec;
      peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
      peer.close(ignore_ec);
    } 

  }
}


void Datanode::write_key_value_to_disk(const std::string& key, const std::string& value){
  try {
    file_.seekg(0, std::ios::end);
    file_.write(value.data(), value.size());
    block_info_[key] = {cur_offset_, value.size()};
    cur_offset_ += value.size();
  } catch(const std::exception& e) {
      std::cerr << e.what() << '\n';
  }

}

std::string Datanode::find_value_by_key_from_disk(const std::string& key) {
  try {
    file_.seekg(block_info_[key].first);
    std::string value;
    value.resize(block_info_[key].second);
    file_.read(value.data(), value.size());
    return value;
  } catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    return "";
  }
 
}



