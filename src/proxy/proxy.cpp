#include "../../include/proxy.h"
#include "../../include/coordinator.h"
#include "../../include/erasure_code.h"
#include <string>
#include <thread>
#include <unordered_map>

Proxy::Proxy(std::string ip, int port)
    : ip_(ip), port_for_rpc_(port + 1000), port_for_transfer_data_(port),
      acceptor_(io_context_, asio::ip::tcp::endpoint(
                                 asio::ip::address::from_string(ip.c_str()),
                                 port_for_transfer_data_)) {
  rpc_server_ = std::make_unique<coro_rpc::coro_rpc_server>(1, port_for_rpc_);
  rpc_server_->register_handler<&Proxy::start_encode_and_store_object>(this);
  rpc_server_->register_handler<&Proxy::decode_and_transfer_data>(this);
  rpc_server_->register_handler<&Proxy::decode_and_transfer_data_concurrence>(this);
  rpc_server_->register_handler<&Proxy::decode_and_transfer_data_CACHED>(this);
  rpc_server_->register_handler<&Proxy::cache_repair>(this);
  rpc_server_->register_handler<&Proxy::main_repair>(this);
  rpc_server_->register_handler<&Proxy::help_repair>(this);
}

Proxy::~Proxy() {
  acceptor_.close();
  rpc_server_->stop();
}

void Proxy::start() { auto err = rpc_server_->start(); }

void Proxy::start_encode_and_store_object(placement_info placement) {
  std::cout<<"when write coordinate connect proxy success"<<std::endl;
  auto encode_and_store = [this, placement]() {
    asio::ip::tcp::socket peer(io_context_);
    acceptor_.accept(peer);

    size_t value_buf_size =
        placement.k * placement.block_size * placement.stripe_ids.size();
    std::string key_buf(placement.key.size(), 0);
    std::string value_buf(value_buf_size, 0);
    my_assert(key_buf.size() == placement.key.size());
    my_assert(value_buf.size() == value_buf_size);
    size_t readed_len_of_key =
        asio::read(peer, asio::buffer(key_buf.data(), key_buf.size()));
    my_assert(readed_len_of_key == key_buf.size());

    size_t readed_len_of_value =
        asio::read(peer, asio::buffer(value_buf.data(), placement.value_len));
    my_assert(readed_len_of_value == placement.value_len);

    char *object_value = value_buf.data();
    for (auto i = 0; i < placement.stripe_ids.size(); i++) {
      std::vector<char *> data_v(placement.k);
      std::vector<char *> coding_v(placement.g + placement.real_l);
      char **data = (char **)data_v.data();
      char **coding = (char **)coding_v.data();

      size_t cur_block_size;
      if ((i == placement.stripe_ids.size() - 1) &&
          placement.tail_block_size != -1) {
        cur_block_size = placement.tail_block_size;
      } else {
        cur_block_size = placement.block_size;
      }
      my_assert(cur_block_size > 0);

      std::vector<std::vector<char>> space_for_parity_blocks(
          placement.g + placement.real_l, std::vector<char>(cur_block_size));
      for (int j = 0; j < placement.k; j++) {
        data[j] = &object_value[j * cur_block_size];
      }
      for (int j = 0; j < placement.g + placement.real_l; j++) {
        coding[j] = space_for_parity_blocks[j].data();
      }

      encode(placement.k, placement.g, placement.real_l, data, coding,
             cur_block_size, placement.encode_type);

      int num_of_datanodes_involved =
          placement.k + placement.g + placement.real_l;
      int num_of_blocks_each_stripe = num_of_datanodes_involved;
      std::vector<std::thread> writers;
      int k = placement.k;
      int g = placement.g;
      int real_l = placement.real_l;
      for (int j = 0; j < num_of_datanodes_involved; j++) {
        std::string block_id =
            std::to_string(placement.stripe_ids[i] * 1000 + j);
        std::pair<std::string, int> ip_and_port_of_datanode =
            placement.datanode_ip_port[i * num_of_blocks_each_stripe + j];
        std::pair<std::string, int> ip_and_port_of_cachenode;
        if( j >= k + g - 1){
          ip_and_port_of_cachenode =
            placement.cachenode_ip_port[i * (1 + real_l) + (j - (k + g -1 ))];
        }

        writers.push_back(
            std::thread([this, j, k, g, real_l, block_id, data, coding, cur_block_size,
                         ip_and_port_of_datanode, ip_and_port_of_cachenode]() {
              
              
              if (j < k) {
                write_to_datanode(block_id.c_str(), block_id.size(), data[j],
                                  cur_block_size,
                                  ip_and_port_of_datanode.first.c_str(),
                                  ip_and_port_of_datanode.second);
              }else if (j >= k && j < (k+g-1)) {
                write_to_datanode(block_id.c_str(), block_id.size(), 
                                  coding[j - k],cur_block_size,
                                  ip_and_port_of_datanode.first.c_str(),
                                  ip_and_port_of_datanode.second);            
              }else {
                write_to_datanode(block_id.c_str(), block_id.size(), 
                                  coding[j - k], cur_block_size,
                                  ip_and_port_of_datanode.first.c_str(),
                                  ip_and_port_of_datanode.second);
                write_to_cachenode(block_id.c_str(), block_id.size(), 
                                  coding[j - k], cur_block_size,
                                  ip_and_port_of_cachenode.first.c_str(),
                                  ip_and_port_of_cachenode.second);
              }
            }));
      }
      for (auto j = 0; j < writers.size(); j++) {
        writers[j].join();
      }

      object_value += (placement.k * cur_block_size);
    }

    std::vector<char> finish(1);
    asio::write(peer, asio::buffer(finish, finish.size()));

    asio::error_code ignore_ec;
    peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
    peer.close(ignore_ec);
  };

  std::thread new_thread(encode_and_store);
  new_thread.detach();
}


void Proxy::write_to_datanode(const char *key, size_t key_len,
                              const char *value, size_t value_len,
                              const char *ip, int port){
  asio::ip::tcp::socket peer(io_context_);
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip), port);
  peer.connect(endpoint);
  int flag = 2;
  std::vector<unsigned char> flag_buf = int_to_bytes(flag);
  asio::write(peer, asio::buffer(flag_buf, flag_buf.size()));

  std::vector<unsigned char> key_size_buf = int_to_bytes(key_len);
  asio::write(peer, asio::buffer(key_size_buf, key_size_buf.size()));

  std::vector<unsigned char> value_size_buf = int_to_bytes(value_len);
  asio::write(peer, asio::buffer(value_size_buf, value_size_buf.size()));

  asio::write(peer, asio::buffer(key, key_len));
  asio::write(peer, asio::buffer(value, value_len));

  std::vector<char> finish(1);
  asio::read(peer, asio::buffer(finish, finish.size()));

  asio::error_code ignore_ec;
  peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
  peer.close(ignore_ec);

}

void Proxy::write_to_cachenode(const char *key, size_t key_len,
                              const char *value, size_t value_len,
                              const char *ip, int port) {
  asio::ip::tcp::socket peer(io_context_);
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip), port);
  peer.connect(endpoint);

  int flag = 0;
  
  std::vector<unsigned char> flag_buf = int_to_bytes(flag);
  asio::write(peer, asio::buffer(flag_buf, flag_buf.size()));
  
  std::vector<unsigned char> key_size_buf = int_to_bytes(key_len);
  asio::write(peer, asio::buffer(key_size_buf, key_size_buf.size()));
  
  std::vector<unsigned char> value_size_buf = int_to_bytes(value_len);
  asio::write(peer, asio::buffer(value_size_buf, value_size_buf.size()));
  
  asio::write(peer, asio::buffer(key, key_len));
  asio::write(peer, asio::buffer(value, value_len));
  
  std::vector<char> finish(1);
  asio::read(peer, asio::buffer(finish, finish.size()));

  asio::error_code ignore_ec;
  peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
  peer.close(ignore_ec);
}



void Proxy::decode_and_transfer_data_concurrence(placement_info placement){
  auto decode_and_transfer = [this, placement](){
    std::string object_value;
    for (auto i = 0; i < placement.stripe_ids.size(); i++) {
      unsigned int stripe_id = placement.stripe_ids[i];
      auto blocks_ptr =
          std::make_shared<std::vector<std::vector<char>>>();
      auto blocks_idx_ptr =
          std::make_shared<std::vector<int>>();
      auto myLock_ptr = std::make_shared<std::mutex>();
      auto cv_ptr = std::make_shared<std::condition_variable>();
      int expect_block_number = placement.k + placement.real_l -1;
      int all_expect_blocks = placement.k + placement.g +placement.real_l ;

      size_t cur_block_size;
      if ((i == placement.stripe_ids.size() - 1) &&
          placement.tail_block_size != -1) {
        cur_block_size = placement.tail_block_size;
      } else {
        cur_block_size = placement.block_size;
      }
      my_assert(cur_block_size > 0);

      std::vector<char *> data_v(placement.k);
      std::vector<char *> coding_v(all_expect_blocks - placement.k);
      char **data = (char **)data_v.data();
      char **coding = (char **)coding_v.data();
      int k = placement.k;
      int g = placement.g;
      int real_l = placement.real_l;

      auto getFromNode=[this, k, g, blocks_ptr, blocks_idx_ptr, myLock_ptr, cv_ptr]
      (int expect_block_number, int stripe_id, int block_idx, int cur_block_size, std::string ip, int port )
      {
        std::string block_id = std::to_string(stripe_id * 1000 + block_idx);
        std::vector<char> block(cur_block_size);

       
        if( block_idx < k + g - 1){
          read_from_datanode(block_id.c_str(), block_id.size(),
                            block.data(), cur_block_size, ip.c_str(), port);
        }else{
          read_from_cachenode(block_id.c_str(), block_id.size(),
                              block.data(), cur_block_size, ip.c_str(), port);
        }
        ///////////////////////////////////////////
        std::cout<<"read success"<<std::endl;

        myLock_ptr->lock();

        if (!check_received_block(k, expect_block_number, blocks_idx_ptr, blocks_ptr->size())){
          blocks_ptr->push_back(block);
          blocks_idx_ptr->push_back(block_idx);
          if (check_received_block(k, expect_block_number,blocks_idx_ptr, blocks_ptr->size())){
            cv_ptr->notify_all();
          }

        }
        myLock_ptr->unlock();
      };

      ////////////////////////////////////////////////////////////////////////////////////
      std::vector<std::vector<char>> space_for_data_blocks(k, std::vector<char>(cur_block_size));
      std::vector<std::vector<char>> space_for_parity_blocks(all_expect_blocks - k , std::vector<char>(cur_block_size));
      for (int j = 0; j < k; j++) {
        data[j] = space_for_data_blocks[j].data();
      }
      for (int j = 0; j < all_expect_blocks - k; j++) {
        coding[j] = space_for_parity_blocks[j].data();
      }

      /////////////////////////////////////////////////////////////////////////////////////
      int num_of_blocks_each_stripe =
          placement.k + placement.g + placement.real_l;
      std::vector<std::thread> readers;
      for (int j = 0; j < all_expect_blocks; j++) {
        std::pair<std::string, int> ip_and_port_of_datanode =
            placement.datanode_ip_port[i * num_of_blocks_each_stripe + j];

        std::pair<std::string, int> ip_and_port_of_cachenode;
        if( j >= k + g - 1){
          ip_and_port_of_cachenode = placement.cachenode_ip_port[i * (1 + real_l) + (j - (k + g -1 ))];
        }
        
        if( j < k + g - 1 ){
          readers.push_back(
            std::thread(getFromNode, expect_block_number, stripe_id, j, cur_block_size, 
                        ip_and_port_of_datanode.first, ip_and_port_of_datanode.second));
        } else{
          readers.push_back(
            std::thread(getFromNode, expect_block_number, stripe_id, j, cur_block_size, 
                        ip_and_port_of_cachenode.first, ip_and_port_of_cachenode.second));
        }                   
      }
      for (auto j = 0; j < all_expect_blocks; j++) {
        readers[j].detach();
      }

      std::unique_lock<std::mutex> lck(*myLock_ptr);

      while(!check_received_block(k, expect_block_number, blocks_idx_ptr, blocks_ptr->size())){
        cv_ptr->wait(lck);
      }
      for(int j = 0; j < int(blocks_idx_ptr->size()); j++){
        int idx = (*blocks_idx_ptr)[j];
        if (idx < k){
          memcpy(data[idx], (*blocks_ptr)[j].data(), cur_block_size);
        }else{
          memcpy(coding[idx - k], (*blocks_ptr)[j].data(), cur_block_size);
        }

      }

      auto erasures = std::make_shared<std::vector<int>>();
      for (int j = 0; j < all_expect_blocks; j++){
        if (std::find(blocks_idx_ptr->begin(), blocks_idx_ptr->end(), j) == blocks_idx_ptr->end()){
          erasures->push_back(j);
        }
      }
      erasures->push_back(-1);

      if (!decode(k, g, real_l, data, coding, erasures, cur_block_size)){
        std::cout << "cannot decode!" << std::endl;
      }

      for (int j = 0; j < k; j++){
        object_value += std::string(data[j], cur_block_size);
      }

          
    }

    asio::ip::tcp::socket peer(io_context_);
    asio::ip::tcp::endpoint endpoint(
        asio::ip::make_address(placement.client_ip), placement.client_port);
    peer.connect(endpoint);

    asio::write(peer, asio::buffer(placement.key, placement.key.size()));
    asio::write(peer, asio::buffer(object_value, object_value.size()));

    asio::error_code ignore_ec;
    peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
    peer.close(ignore_ec);


  };



  std::thread new_thread(decode_and_transfer);
  new_thread.detach();
}






void Proxy::decode_and_transfer_data_CACHED(placement_info placement){
  auto decode_and_transfer = [this, placement]() {
    std::string object_value;
    for (auto i = 0; i < placement.stripe_ids.size(); i++) {
      unsigned int stripe_id = placement.stripe_ids[i];
      auto blocks_ptr =
          std::make_shared<std::vector<std::vector<char>>>();
      auto blocks_idx_ptr =
          std::make_shared<std::vector<int>>();
      auto myLock_ptr = 
          std::make_shared<std::mutex>();
      auto cv_ptr =
          std::make_shared<std::condition_variable>();
      int expect_block_number = placement.k;
      int all_expect_blocks = placement.k + placement.g + placement.real_l;
      std::vector<char *> data_v(placement.k);
      std::vector<char *> coding_v(all_expect_blocks - placement.k);
      char **data = (char **)data_v.data();
      char **coding = (char **)coding_v.data();
      int k = placement.k;
      int g = placement.g;
      int real_l = placement.real_l;

      auto getFromNode = [this, k, g, blocks_ptr, blocks_idx_ptr, myLock_ptr, cv_ptr, placement]
                          (int expect_block_number, int stripe_id_, int block_idx, 
                           int n_block_size, std::string ip, int port){
        std::string block_id = std::to_string(stripe_id_ * 1000 + block_idx);
        std::vector<char> temp(n_block_size);
        if( block_idx < k + g - 1){
          read_from_datanode(block_id.c_str(), block_id.size(),
                            temp.data(), n_block_size, ip.c_str(), port);
        }else{
          read_from_cachenode(block_id.c_str(), block_id.size(),
                              temp.data(), n_block_size, ip.c_str(), port);
        }
        
        myLock_ptr->lock();

        if (!check_received_block(k, expect_block_number, blocks_idx_ptr, blocks_ptr->size())){
          blocks_ptr->push_back(temp);
          blocks_idx_ptr->push_back(block_idx);
          if(check_received_block(k, expect_block_number, blocks_idx_ptr, blocks_ptr->size())){
            cv_ptr->notify_all();
          }
        }
        myLock_ptr->unlock();
      };

      size_t cur_block_size;
      if ((i == placement.stripe_ids.size() - 1) &&
          placement.tail_block_size != -1) {
        cur_block_size = placement.tail_block_size;
      } else {
        cur_block_size = placement.block_size;
      }
      my_assert(cur_block_size > 0);

      std::vector<std::vector<char>> space_for_data_blocks(
          k, std::vector<char>(cur_block_size));
      std::vector<std::vector<char>> space_for_parity_blocks(
          all_expect_blocks-k, std::vector<char>(cur_block_size));
      for(int j = 0; j < k; j++){
        data[j] = space_for_data_blocks[j].data();
      }
      for(int j = 0; j < all_expect_blocks - k; j++){
        coding[j] = space_for_parity_blocks[j].data();
      }

      std::vector<std::thread> read_treads;
      int num_of_blocks_each_stripe =
          placement.k + placement.g + placement.real_l;
      for(int j = 0; j < all_expect_blocks; j++){
          std::pair<std::string, int> ip_and_port_of_datanode =
                  placement.datanode_ip_port[i * num_of_blocks_each_stripe + j];
          std::pair<std::string, int> ip_and_port_of_cachenode =
                  placement.cachenode_ip_port[i * (1 + real_l) + (j - (k + g -1 ))];
          if( j < k + g - 1 ){
            read_treads.push_back(std::thread(
                getFromNode, expect_block_number, stripe_id, j, cur_block_size, 
                ip_and_port_of_datanode.first, ip_and_port_of_datanode.second));
          }else{
            read_treads.push_back(std::thread(
                getFromNode, expect_block_number, stripe_id, j, cur_block_size, 
                ip_and_port_of_cachenode.first, ip_and_port_of_cachenode.second));
          }                 
      }
      for(int j = 0; j< all_expect_blocks; j++){
        read_treads[j].detach();
      }

      std::unique_lock<std::mutex> lck(*myLock_ptr);
      
      while(!check_received_block(k, expect_block_number, blocks_idx_ptr, blocks_ptr->size())){
        cv_ptr->wait(lck);
      }

      for(int j = 0; j < int(blocks_idx_ptr->size()); j++){
        int idx = (*blocks_idx_ptr)[j];
        if(idx < k){
          memcpy(data[idx], (*blocks_ptr)[j].data(), cur_block_size);
        }else{
          memcpy(coding[idx-k], (*blocks_ptr)[j].data(), cur_block_size);
        }
      }

      auto erasures = std::make_shared<std::vector<int>>();
      for(int j = 0; j < all_expect_blocks; j++){
        if(std::find(blocks_idx_ptr->begin(), blocks_idx_ptr->end(), j) == blocks_idx_ptr->end()){
          erasures->push_back(j);        
        }
      }
      erasures->push_back(-1);
      decode(k, g, real_l, data, coding, erasures, cur_block_size, true);

      for(int j = 0; j < k; j++){
        object_value += std::string(data[j], cur_block_size);

      }
     
    }

    asio::ip::tcp::socket peer(io_context_);
    asio::ip::tcp::endpoint endpoint(
        asio::ip::make_address(placement.client_ip), placement.client_port);
    peer.connect(endpoint);

    asio::write(peer, asio::buffer(placement.key, placement.key.size()));
    asio::write(peer, asio::buffer(object_value, object_value.size()));

    asio::error_code ignore_ec;
    peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
    peer.close(ignore_ec);
  };

  std::thread new_thread(decode_and_transfer);
  new_thread.detach();

  
}

void Proxy::decode_and_transfer_data(placement_info placement) {
  auto decode_and_transfer = [this, placement]() {
    std::string object_value;
    for (auto i = 0; i < placement.stripe_ids.size(); i++) {
      unsigned int stripe_id = placement.stripe_ids[i];
      auto blocks_ptr =
          std::make_shared<std::unordered_map<int, std::string>>();

      size_t cur_block_size;
      if ((i == placement.stripe_ids.size() - 1) &&
          placement.tail_block_size != -1) {
        cur_block_size = placement.tail_block_size;
      } else {
        cur_block_size = placement.block_size;
      }
      my_assert(cur_block_size > 0);

      int num_of_datanodes_involved = placement.k;
      int num_of_blocks_each_stripe =
          placement.k + placement.g + placement.real_l;
      std::vector<std::thread> readers;
      for (int j = 0; j < num_of_datanodes_involved; j++) {
        std::pair<std::string, int> ip_and_port_of_datanode =
            placement.datanode_ip_port[i * num_of_blocks_each_stripe + j];
        readers.push_back(
            std::thread([this, j, stripe_id, blocks_ptr, cur_block_size,
                         ip_and_port_of_datanode]() {
              std::string block_id = std::to_string(stripe_id * 1000 + j);
              std::string block(cur_block_size, 0);
              read_from_datanode(block_id.c_str(), block_id.size(),
                                 block.data(), cur_block_size,
                                 ip_and_port_of_datanode.first.c_str(),
                                 ip_and_port_of_datanode.second);

              mutex_.lock();

              (*blocks_ptr)[j] = block;

              mutex_.unlock();
            }));
      }
      for (auto j = 0; j < readers.size(); j++) {
        readers[j].join();
      }

      my_assert(blocks_ptr->size() == num_of_datanodes_involved);

      for (int j = 0; j < placement.k; j++) {
        object_value += (*blocks_ptr)[j];
      }
    }

    asio::ip::tcp::socket peer(io_context_);
    asio::ip::tcp::endpoint endpoint(
        asio::ip::make_address(placement.client_ip), placement.client_port);
    peer.connect(endpoint);

    asio::write(peer, asio::buffer(placement.key, placement.key.size()));
    asio::write(peer, asio::buffer(object_value, object_value.size()));

    asio::error_code ignore_ec;
    peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
    peer.close(ignore_ec);
  };

  std::thread new_thread(decode_and_transfer);
  new_thread.detach();
}



void Proxy::read_from_cachenode(const char *key, size_t key_len, char *value,
                               size_t value_len, const char *ip, int port) {
  asio::ip::tcp::socket peer(io_context_);
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip), port);
  peer.connect(endpoint);

  int flag = 1;
  std::vector<unsigned char> flag_buf = int_to_bytes(flag);
  asio::write(peer, asio::buffer(flag_buf, flag_buf.size()));

  std::vector<unsigned char> key_size_buf = int_to_bytes(key_len);
  asio::write(peer, asio::buffer(key_size_buf, key_size_buf.size()));

  asio::write(peer, asio::buffer(key, key_len));

  asio::read(peer, asio::buffer(value, value_len));

  asio::error_code ignore_ec;
  peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
  peer.close(ignore_ec);
}


void Proxy::read_from_datanode(const char *key, size_t key_len, char *value,
                               size_t value_len, const char *ip, int port) {
  asio::ip::tcp::socket peer(io_context_);
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(ip), port);
  peer.connect(endpoint);
  int flag = 3;
  std::vector<unsigned char> flag_buf = int_to_bytes(flag);
  asio::write(peer, asio::buffer(flag_buf, flag_buf.size()));

  std::vector<unsigned char> key_size_buf = int_to_bytes(key_len);
  asio::write(peer, asio::buffer(key_size_buf, key_size_buf.size()));

  asio::write(peer, asio::buffer(key, key_len));

  asio::read(peer, asio::buffer(value, value_len));

  asio::error_code ignore_ec;
  peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
  peer.close(ignore_ec);
}


///////////////////////////////////////
///////////////////////////////////////
///////////////////////////////////////////////////////////
void Proxy::cache_repair(main_repair_plan repair_plan) {
  int failed_block_index = repair_plan.failed_blocks_index[0];
  
 
    std::string &ip =
        repair_plan.inner_cluster_help_blocks_info[0].first.first;
    int port = repair_plan.inner_cluster_help_blocks_info[0].first.second;
    int block_index = repair_plan.inner_cluster_help_blocks_info[0].second;
    std::vector<char> block_buf(repair_plan.block_size);

    std::string block_id =
        std::to_string(repair_plan.stripe_id * 1000 + block_index);
    
    read_from_cachenode(block_id.c_str(), block_id.size(), block_buf.data(),
                         repair_plan.block_size, ip.c_str(), port);
    
  //}

  std::vector<char> repaired_block(repair_plan.block_size);
  repaired_block = block_buf;

  std::string ip_repair = repair_plan.new_locations[0].first.first;
  int port_repair = repair_plan.new_locations[0].first.second;
  std::string key = std::to_string(repair_plan.stripe_id * 1000 +
                                   repair_plan.failed_blocks_index[0]);
  write_to_datanode(key.data(), key.size(), repaired_block.data(),
                    repaired_block.size(), ip_repair.c_str(), port_repair);


}




void Proxy::main_repair(main_repair_plan repair_plan) {
  my_assert(repair_plan.failed_blocks_index.size() == 1);

  std::sort(repair_plan.live_blocks_index.begin(),
            repair_plan.live_blocks_index.end());
  std::sort(repair_plan.failed_blocks_index.begin(),
            repair_plan.failed_blocks_index.end());

  std::vector<std::thread> readers_inner_cluster;
  std::vector<std::thread> readers_outter_cluster;

  int failed_block_index = repair_plan.failed_blocks_index[0];

  std::unordered_map<unsigned int, std::unordered_map<int, std::vector<char>>>
      blocks;

  for (auto i = 0; i < repair_plan.help_cluster_ids.size(); i++) {
    readers_outter_cluster.push_back(std::thread([&, this]() {
      mutex_.lock();
      asio::ip::tcp::socket peer(io_context_);
      acceptor_.accept(peer);
      mutex_.unlock();

      std::vector<unsigned char> cluster_id_buf(sizeof(int));
      asio::read(peer, asio::buffer(cluster_id_buf, cluster_id_buf.size()));
      int help_cluster_id = bytes_to_int(cluster_id_buf);

      if (failed_block_index >= repair_plan.k &&
          failed_block_index <= (repair_plan.k + repair_plan.g - 1)) {

        std::vector<unsigned char> num_of_blocks_buf(sizeof(int));
        asio::read(peer,
                   asio::buffer(num_of_blocks_buf, num_of_blocks_buf.size()));
        int num_of_blocks = bytes_to_int(num_of_blocks_buf);

        for (int j = 0; j < num_of_blocks; j++) {
          std::vector<unsigned char> block_index_buf(sizeof(int));
          asio::read(peer,
                     asio::buffer(block_index_buf, block_index_buf.size()));
          int block_index = bytes_to_int(block_index_buf);

          std::vector<char> block_buf(repair_plan.block_size);
          asio::read(peer, asio::buffer(block_buf, block_buf.size()));

          mutex_.lock();
          blocks[help_cluster_id][block_index] = block_buf;
          mutex_.unlock();
        }
      } else {


        std::vector<char> block_buf(repair_plan.block_size);
        asio::read(peer, asio::buffer(block_buf, block_buf.size()));
        mutex_.lock();
        blocks[help_cluster_id][-1] = block_buf;
        mutex_.unlock();
      }

      asio::error_code ignore_ec;
      peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
      peer.close(ignore_ec);
    }));
  }
  for (auto i = 0; i < readers_outter_cluster.size(); i++) {
    readers_outter_cluster[i].join();
  }

  for (auto i = 0; i < repair_plan.inner_cluster_help_blocks_info.size(); i++) {
    readers_inner_cluster.push_back(std::thread([&, this, i]() {
      std::string &ip =
          repair_plan.inner_cluster_help_blocks_info[i].first.first;
      int port = repair_plan.inner_cluster_help_blocks_info[i].first.second;
      int block_index = repair_plan.inner_cluster_help_blocks_info[i].second;
      int k = repair_plan.k;
      int g = repair_plan.g;
      std::vector<char> block_buf(repair_plan.block_size);
      std::string block_id =
          std::to_string(repair_plan.stripe_id * 1000 + block_index);
      size_t temp_size;

      if(block_index < k + g -1){
        read_from_datanode(block_id.c_str(), block_id.size(), block_buf.data(),
                         repair_plan.block_size, ip.c_str(), port);
      }else{
        read_from_cachenode(block_id.c_str(), block_id.size(), block_buf.data(),
                         repair_plan.block_size, ip.c_str(), port);
      }
      mutex_.lock();
      blocks[repair_plan.cluster_id][block_index] = block_buf;
      mutex_.unlock();
    }));
  }
  for (auto i = 0; i < readers_inner_cluster.size(); i++) {
    readers_inner_cluster[i].join();
  }

  std::vector<char> repaired_block(repair_plan.block_size);
  int k = repair_plan.k;
  int real_l = repair_plan.real_l;
  int b = repair_plan.b;
  int g = repair_plan.g;
  if (failed_block_index >= k && failed_block_index <= (k + g - 1)) {

    std::vector<int> matrix;
    matrix.resize((g + real_l) * k);
    make_lrc_coding_matrix(k, g, real_l, matrix.data());

    std::vector<int> full_matrix((k + g + real_l) * k, 0);
    for (int i = 0; i < (k + g + real_l); i++) {
      if (i < k) {
        full_matrix[i * k + i] = 1;
      } else {
        for (int j = 0; j < k; j++) {
          full_matrix[i * k + j] = matrix[(i - k) * k + j];
        }
      }
    }

    std::vector<int> matrix_failed_block;
    matrix_failed_block.resize(repair_plan.failed_blocks_index.size() * k);
    for (auto i = 0; i < repair_plan.failed_blocks_index.size(); i++) {
      int row = repair_plan.failed_blocks_index[i];
      int *coff = &(full_matrix[row * k]);
      for (int j = 0; j < k; j++) {
        matrix_failed_block[i * k + j] = coff[j];
      }
    }

    std::vector<int> matrix_live_block;
    matrix_live_block.resize(k * k);
    for (auto i = 0; i < repair_plan.live_blocks_index.size(); i++) {
      int row = repair_plan.live_blocks_index[i];
      int *coff = &(full_matrix[row * k]);
      for (int j = 0; j < k; j++) {
        matrix_live_block[i * k + j] = coff[j];
      }
    }

    std::vector<int> invert_matrix_live_block;
    invert_matrix_live_block.resize(k * k);
    jerasure_invert_matrix(matrix_live_block.data(),
                           invert_matrix_live_block.data(), k, 8);

 
    int *help_matrix_ptr = jerasure_matrix_multiply(
        matrix_failed_block.data(), invert_matrix_live_block.data(),
        repair_plan.failed_blocks_index.size(), k, k, k, 8);
    std::vector<int> help_matrix;
    help_matrix.resize(repair_plan.failed_blocks_index.size() * k);
    memcpy(help_matrix.data(), help_matrix_ptr,
           help_matrix.size() * sizeof(int));
    free(help_matrix_ptr);

    for (auto &blocks_in_each_cluster : blocks) {
      if (blocks_in_each_cluster.first == repair_plan.cluster_id) {


        std::vector<char> encode_result(repair_plan.block_size);
        std::vector<std::pair<int, std::vector<char>>> saved_encode_result;
        int num_of_live_blocks_in_cur_cluster =
            blocks_in_each_cluster.second.size();
        std::vector<char *> data_v(num_of_live_blocks_in_cur_cluster);
        std::vector<char *> coding_v(1);
        char **data = (char **)data_v.data();
        char **coding = (char **)coding_v.data();

        for (auto i = 0; i < repair_plan.failed_blocks_index.size(); i++) {
          int *coff = &(help_matrix[i * k]);
          std::vector<int> coding_matrix(1 * num_of_live_blocks_in_cur_cluster,
                                         1);
          int idx = 0;
          for (auto &block : blocks_in_each_cluster.second) {
            int coff_idx = 0;
            for (; coff_idx < repair_plan.live_blocks_index.size();
                 coff_idx++) {
              if (repair_plan.live_blocks_index[coff_idx] == block.first) {
                break;
              }
            }
            coding_matrix[idx] = coff[coff_idx];
            data[idx] = block.second.data();
            idx++;
          }

          int sum = 0;
          for (auto &num : coding_matrix) {
            sum += num;
          }

          coding[0] = encode_result.data();
          jerasure_matrix_encode(num_of_live_blocks_in_cur_cluster, 1, 8,
                                 coding_matrix.data(), data, coding,
                                 repair_plan.block_size);

          if (sum == 0) {
            encode_result = std::vector<char>(repair_plan.block_size, 0);
          }

          saved_encode_result.push_back(
              {repair_plan.failed_blocks_index[i], encode_result});
        }
        blocks_in_each_cluster.second.clear();
        for (auto &encode_result : saved_encode_result) {
          blocks_in_each_cluster.second[encode_result.first] =
              encode_result.second;
        }
      }
    }

    for (auto i = 0; i < repair_plan.failed_blocks_index.size(); i++) {
      int num_of_clusters_involved = blocks.size();
      std::vector<char *> data_v(num_of_clusters_involved);
      std::vector<char *> coding_v(1);
      char **data = (char **)data_v.data();
      char **coding = (char **)coding_v.data();
      int idx = 0;
      for (auto &blocks_in_each_cluster : blocks) {
        data[idx++] =
            blocks_in_each_cluster.second[repair_plan.failed_blocks_index[i]]
                .data();
      }
      coding[0] = repaired_block.data();
      std::vector<int> new_matrix(1 * num_of_clusters_involved, 1);
      jerasure_matrix_encode(num_of_clusters_involved, 1, 8, new_matrix.data(),
                             data, coding, repair_plan.block_size);
    }
  } else {

    int num_of_blocks_involved = 0;
    for (auto &blocks_in_each_cluster : blocks) {
      num_of_blocks_involved += blocks_in_each_cluster.second.size();
    }

    std::vector<char *> data_v(num_of_blocks_involved);
    std::vector<char *> coding_v(1);
    char **data = (char **)data_v.data();
    char **coding = (char **)coding_v.data();

    int idx = 0;
    for (auto &num_of_blocks_involved : blocks) {
      for (auto &block : num_of_blocks_involved.second) {
        data[idx++] = block.second.data();
      }
    }
    coding[0] = repaired_block.data();
    std::vector<int> new_matrix(1 * num_of_blocks_involved, 1);

    jerasure_matrix_encode(num_of_blocks_involved, 1, 8, new_matrix.data(),
                           data, coding, repair_plan.block_size);
  }

  my_assert(repair_plan.new_locations.size() == 1);
  std::string ip = repair_plan.new_locations[0].first.first;
  int port = repair_plan.new_locations[0].first.second;
  std::string key = std::to_string(repair_plan.stripe_id * 1000 +
                                   repair_plan.failed_blocks_index[0]);
  write_to_datanode(key.data(), key.size(), repaired_block.data(),
                    repaired_block.size(), ip.c_str(), port);
}

void Proxy::help_repair(help_repair_plan repair_plan) {
  std::sort(repair_plan.live_blocks_index.begin(),
            repair_plan.live_blocks_index.end());
  std::sort(repair_plan.failed_blocks_index.begin(),
            repair_plan.failed_blocks_index.end());

  std::unordered_map<unsigned int, std::unordered_map<int, std::vector<char>>>
      blocks;

  std::vector<std::thread> readers_inner_cluster;
  for (auto i = 0; i < repair_plan.inner_cluster_help_blocks_info.size(); i++) {
    readers_inner_cluster.push_back(std::thread([&, this, i]() {
      std::string &ip =
          repair_plan.inner_cluster_help_blocks_info[i].first.first;
      int port = repair_plan.inner_cluster_help_blocks_info[i].first.second;
      int block_idx = repair_plan.inner_cluster_help_blocks_info[i].second;
      int k = repair_plan.k;
      int g = repair_plan.g;
      std::vector<char> buf(repair_plan.block_size);
      std::string block_id =
          std::to_string(repair_plan.stripe_id * 1000 + block_idx);
      size_t temp_size;
      if(block_idx < k + g -1){
        read_from_datanode(block_id.c_str(), block_id.size(), buf.data(),
                         repair_plan.block_size, ip.c_str(), port);
      }else{
        read_from_cachenode(block_id.c_str(), block_id.size(), buf.data(),
                         repair_plan.block_size, ip.c_str(), port);
      }
      mutex_.lock();
      blocks[repair_plan.cluster_id][block_idx] = buf;
      mutex_.unlock();
    }));
  }
  for (auto &thread : readers_inner_cluster) {
    thread.join();
  }

  asio::ip::tcp::socket peer(io_context_);
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address(repair_plan.main_proxy_ip),
                                   repair_plan.main_proxy_port);
  peer.connect(endpoint);

  std::vector<unsigned char> cluster_id_buf =
      int_to_bytes(repair_plan.cluster_id);
  asio::write(peer, asio::buffer(cluster_id_buf, cluster_id_buf.size()));

  int k = repair_plan.k;
  int real_l = repair_plan.real_l;
  int b = repair_plan.b;
  int g = repair_plan.g;

  my_assert(repair_plan.failed_blocks_index.size() == 1);
  int failed_blocks_index = repair_plan.failed_blocks_index[0];
  if (failed_blocks_index >= k && failed_blocks_index <= (k + g - 1)) {

    std::vector<unsigned char> num_of_blocks_buf =
        int_to_bytes(repair_plan.failed_blocks_index.size());
    asio::write(peer,
                asio::buffer(num_of_blocks_buf, num_of_blocks_buf.size()));

    std::vector<int> matrix;
    matrix.resize((g + real_l) * k);
    make_lrc_coding_matrix(k, g, real_l, matrix.data());

    std::vector<int> full_matrix((k + g + real_l) * k, 0);
    for (int i = 0; i < (k + g + real_l); i++) {
      if (i < k) {
        full_matrix[i * k + i] = 1;
      } else {
        for (int j = 0; j < k; j++) {
          full_matrix[i * k + j] = matrix[(i - k) * k + j];
        }
      }
    }


    std::vector<int> matrix_failed_block;
    matrix_failed_block.resize(repair_plan.failed_blocks_index.size() * k);
    for (auto i = 0; i < repair_plan.failed_blocks_index.size(); i++) {
      int row = repair_plan.failed_blocks_index[i];
      int *coff = &(full_matrix[row * k]);
      for (int j = 0; j < k; j++) {
        matrix_failed_block[i * k + j] = coff[j];
      }
    }

    std::vector<int> matrix_live_block;
    matrix_live_block.resize(k * k);
    for (auto i = 0; i < repair_plan.live_blocks_index.size(); i++) {
      int row = repair_plan.live_blocks_index[i];
      int *coff = &(full_matrix[row * k]);
      for (int j = 0; j < k; j++) {
        matrix_live_block[i * k + j] = coff[j];
      }
    }

    std::vector<int> invert_matrix_live_block;
    invert_matrix_live_block.resize(k * k);
    jerasure_invert_matrix(matrix_live_block.data(),
                           invert_matrix_live_block.data(), k, 8);

    int *help_matrix_ptr = jerasure_matrix_multiply(
        matrix_failed_block.data(), invert_matrix_live_block.data(),
        repair_plan.failed_blocks_index.size(), k, k, k, 8);
    std::vector<int> help_matrix;
    help_matrix.resize(repair_plan.failed_blocks_index.size() * k);
    memcpy(help_matrix.data(), help_matrix_ptr,
           help_matrix.size() * sizeof(int));
    free(help_matrix_ptr);

    for (auto &blocks_in_each_cluster : blocks) {
      if (blocks_in_each_cluster.first == repair_plan.cluster_id) {

        std::vector<char> encode_result(repair_plan.block_size);
        std::vector<std::pair<int, std::vector<char>>> saved_encode_result;
        int num_of_live_blocks_in_cur_cluster =
            blocks_in_each_cluster.second.size();
        std::vector<char *> data_v(num_of_live_blocks_in_cur_cluster);
        std::vector<char *> coding_v(1);
        char **data = (char **)data_v.data();
        char **coding = (char **)coding_v.data();

        for (auto i = 0; i < repair_plan.failed_blocks_index.size(); i++) {
          int *coff = &(help_matrix[i * k]);
          std::vector<int> coding_matrix(1 * num_of_live_blocks_in_cur_cluster,
                                         1);
          int idx = 0;
          for (auto &block : blocks_in_each_cluster.second) {
            int coff_idx = 0;
            for (; coff_idx < repair_plan.live_blocks_index.size();
                 coff_idx++) {
              if (repair_plan.live_blocks_index[coff_idx] == block.first) {
                break;
              }
            }
            coding_matrix[idx] = coff[coff_idx];
            data[idx] = block.second.data();
            idx++;
          }

          int sum = 0;
          for (auto &num : coding_matrix) {
            sum += num;
          }

          coding[0] = encode_result.data();
          jerasure_matrix_encode(num_of_live_blocks_in_cur_cluster, 1, 8,
                                 coding_matrix.data(), data, coding,
                                 repair_plan.block_size);

          if (sum == 0) {
            encode_result = std::vector<char>(repair_plan.block_size, 0);
          }

          std::vector<unsigned char> block_index_buf =
              int_to_bytes(repair_plan.failed_blocks_index[i]);
          asio::write(peer,
                      asio::buffer(block_index_buf, block_index_buf.size()));
          asio::write(peer, asio::buffer(encode_result, encode_result.size()));
        }
      }
    }
  } else {

    std::vector<char> encode_result(repair_plan.block_size, 1);
    int num_of_blocks_involved = 0;
    for (auto &blocks_in_each_cluster : blocks) {
      num_of_blocks_involved += blocks_in_each_cluster.second.size();
    }

    std::vector<char *> data_v(num_of_blocks_involved);
    std::vector<char *> coding_v(1);
    char **data = (char **)data_v.data();
    char **coding = (char **)coding_v.data();

    int idx = 0;
    for (auto &num_of_blocks_involved : blocks) {
      for (auto &block : num_of_blocks_involved.second) {
        data[idx++] = block.second.data();
      }
    }
    coding[0] = encode_result.data();
    std::vector<int> new_matrix(1 * num_of_blocks_involved, 1);

    jerasure_matrix_encode(num_of_blocks_involved, 1, 8, new_matrix.data(),
                           data, coding, repair_plan.block_size);

    asio::write(peer, asio::buffer(encode_result, encode_result.size()));
  }

  asio::error_code ignore_ec;
  peer.shutdown(asio::ip::tcp::socket::shutdown_both, ignore_ec);
  peer.close(ignore_ec);
}