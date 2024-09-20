#pragma once

#include "asio.hpp"
#include "utils.h"
#include <string>
#include <sw/redis++/redis++.h>
#include <fstream>
#include <mutex>

class Datanode {
public:
  Datanode(std::string ip, int port);
  ~Datanode();
  void keep_working();
  void write_key_value_to_disk(const std::string& key, const std::string& value);
  std::string find_value_by_key_from_disk(const std::string& key);
  std::string filename;

private:
  std::fstream file_;
  std::unordered_map<std::string, std::pair<int, int>> block_info_;
  int cur_offset_;
  std::string ip_;
  int port_;
  asio::io_context io_context_{};
  asio::ip::tcp::acceptor acceptor_;
  std::unique_ptr<sw::redis::Redis> redis_{nullptr};
  //std::unordered_map<std::string, std::string>data;
  std::mutex file_write_mutex;
  std::mutex file_read_mutex;


};