#!/usr/bin/env ruby
require "date"
require "bigdecimal"

# Catalogue Server ID
$server_number = 0
# ラウンドロビンでノードをサーバに分ける
$network_filename = ARGV[0]
$server_num = 72
# 作成したいデータベースとテーブルの名前
$database_name = "cfec_database2"
$neighbor_node_table_name = "neighbor_nodes"
$c_value_table_name = "c_values"
$ip_node_table_name = "ip_nodes"
$now_time
$arr_ip = ["10.58.58.2", "10.58.58.3", "10.58.58.4", "10.58.58.5", "10.58.58.6", "10.58.58.7", "10.58.58.8", "10.58.58.9", "10.58.58.10", "10.58.58.11", "10.58.58.12", "10.58.58.13", "10.58.58.18", "10.58.58.19", "10.58.58.20", "10.58.58.21", "10.58.58.22", "10.58.58.23", "10.58.58.24", "10.58.58.25", "10.58.58.26", "10.58.58.27", "10.58.58.28", "10.58.58.29", "10.58.58.98", "10.58.58.99", "10.58.58.100", "10.58.58.101", "10.58.58.102", "10.58.58.103", "10.58.58.104", "10.58.58.105", "10.58.58.106", "10.58.58.107", "10.58.58.108", "10.58.58.109", "10.58.58.34", "10.58.58.35", "10.58.58.36", "10.58.58.37", "10.58.58.38", "10.58.58.39", "10.58.58.40", "10.58.58.41", "10.58.58.42", "10.58.58.43", "10.58.58.44", "10.58.58.45", "10.58.58.82", "10.58.58.83", "10.58.58.84", "10.58.58.85", "10.58.58.86", "10.58.58.87", "10.58.58.88", "10.58.58.89", "10.58.58.90", "10.58.58.91", "10.58.58.92", "10.58.58.93", "10.58.58.114", "10.58.58.115", "10.58.58.116", "10.58.58.117", "10.58.58.118", "10.58.58.119", "10.58.58.120", "10.58.58.121", "10.58.58.122", "10.58.58.123", "10.58.58.124", "10.58.58.125"]
$hash_node = {}
class Node
  @id
  @arr_neighbor_node
  @server_id
  def initialize(id)
    @id = id
    # hashじゃなくて配列
    @arr_neighbor_node = []
  end
  def id()
    return @id
  end
  def server_id()
    return @server_id
  end
  def setServerId(server_id)
    @server_id = server_id
  end
  def arr_neighbor_node()
    return @arr_neighbor_node
  end
  def oneHopPush(other_id)
    @arr_neighbor_node.push(other_id)
  end
  def degree()
    return @arr_neighbor_node.length
  end
end

def make_database()
  command = "mysql -uroot -e 'create database #{$database_name}'"
  if !system(command) then
    STDERR.puts "データベースを作れませんでした。"
  end
end

def make_tables()
  command = "mysql -uroot -e 'create table #{$database_name}.#{$neighbor_node_table_name} (own_content_id varchar(33), other_content_id varchar(33), version_id varchar(25), next_server_ip varchar(33))'"
  if !system(command) then
    STDERR.puts "#{$neighbor_node_table_name}を作れませんでした。"
  end

  command = "mysql -uroot -e 'create table #{$database_name}.#{$c_value_table_name} (own_content_id varchar(33), other_content_id varchar(33), version_id varchar(25), hop int, next_value double(10, 5), value_chain varchar(257), path_chain varchar(199), recv_time_stamp varchar(13))'"
  if !system(command) then
    STDERR.puts "#{$c_value_table_name}を作れませんでした。"
  end
end
def make_ip_tables()
  command = "mysql -uroot -e 'create table #{$database_name}.#{$c_value_table_name} (own_content_id varchar(33), other_content_id varchar(33), version_id varchar(25), value_chain varchar(257), path_chain varchar(199), recv_time_stamp varchar(13))'"
  if !system(command) then
    STDERR.puts "#{$c_value_table_name}を作れませんでした。"
  end
end

def conv_fileid_to_catid(node, server_id)
  cat_server_id = (server_id.to_s(16)).rjust(8, "0") 
  node_index = (node.to_i.to_s(16)).rjust(8, "0")
  catid = cat_server_id + "00000000" + "00000000" + node_index

  return catid
end

def insert_neighbor_node_table(own_node, other_node, ip, other_node_server_id)
  own_node_id = conv_fileid_to_catid(own_node, $server_number)
  other_node_id = conv_fileid_to_catid(other_node, other_node_server_id)
  time_str = "#{$now_time.year}#{$now_time.month}#{($now_time.day.to_s).rjust(2, "0")}#{$now_time.hour}#{$now_time.min}"
  version_id = ($server_num.to_s(16)).rjust(8, "0") + time_str

  command = "mysql -uroot -e 'insert into #{$database_name}.#{$neighbor_node_table_name} (own_content_id, other_content_id, version_id, next_server_ip) values (\"#{own_node_id}\", \"#{other_node_id}\", \"#{version_id}\", \"#{ip}\")'" 
  if !system(command) then
    STDERR.puts "#{command}を実行できませんでした。"
  end
end

def delete_column_from_neighbor_node_table()
  command = "mysql -phige@mos -uroot -e 'delete from #{$database_name}.#{$neighbor_node_table_name}'"
  if !system(command) then
    STDERR.puts "#{command}を実行できませんでした。"
  end
end

def insert_c_value_table(own_node, other_node, other_node_server_id)
  own_node_id = conv_fileid_to_catid(own_node, $server_number)
  other_node_id = conv_fileid_to_catid(other_node, other_node_server_id)
  time_str = "#{$now_time.year}#{$now_time.month}#{($now_time.day.to_s).rjust(2, "0")}#{$now_time.hour}#{$now_time.min}"
  version_id = ($server_num.to_s(16)).rjust(8, "0") + time_str
  next_value = 1000.0 / ($hash_node[own_node].degree())
  command = "mysql -uroot -e 'insert into #{$database_name}.#{$c_value_table_name} (own_content_id, other_content_id, version_id, hop, next_value, value_chain, path_chain, recv_time_stamp) values (\"#{own_node_id}\", \"#{other_node_id}\", \"#{version_id}\", 0, #{next_value}, \"1000\", \"NULL\", \"#{time_str}\")'" 
  if !system(command) then
    STDERR.puts "#{command}を実行できませんでした。"
  end
end

def load_nodeid_round_robin()
  puts $network_filename
  ($hash_node.sort).each_with_index {|id_node_pair, i|
    id_node_pair[1].setServerId(i % $server_num)
  }
  ($hash_node.sort).each_with_index {|id_node_pair, i|
    puts id_node_pair[0]
    if (i % $server_num) == $server_number.to_i then
      id_node_pair[1].arr_neighbor_node.each {|other_node_id|
        insert_neighbor_node_table(id_node_pair[0], other_node_id, $arr_ip[$hash_node[other_node_id].server_id()], $hash_node[other_node_id].server_id())
        #insert_c_value_table(id_node_pair[0], other_node_id, $hash_node[other_node_id].server_id())
      }
    end
  }
end

def load_node()
  # ネットワークファイルを読み込み
  File.open($network_filename) do |file|
    # 各行を分割
    file.each_line do |line|
      arr_edge = line.split(" ")
      # ノードが既にあればノードの初期化はしない
      if $hash_node.has_key?(arr_edge[0].to_i) then
      else
        $hash_node[arr_edge[0].to_i] = Node.new(arr_edge[0].to_i)
      end
      $hash_node[arr_edge[0].to_i].oneHopPush(arr_edge[1].to_i)
      # テストデータは片方向のため、相互のノードを初期化する
      if $hash_node.has_key?(arr_edge[1].to_i) then
      else
        $hash_node[arr_edge[1].to_i] = Node.new(arr_edge[1].to_i)
      end
      $hash_node[arr_edge[1].to_i].oneHopPush(arr_edge[0].to_i)
    end
  end
end

def start()
  $now_time = DateTime.now
end

start()
#delete_column_from_neighbor_node_table()
#make_database()
#make_tables()
load_node()
load_nodeid_round_robin()
#p $hash_node[0]
#p $hash_node[0].arr_neighbor_node
