#!/usr/bin/env ruby
require "date"
require "bigdecimal"

# Catalogue Server ID
$server_number = 0
# ラウンドロビンでノードをサーバに分ける
$network_filename = ARGV[0]
$server_num = 64
# 作成したいデータベースとテーブルの名前
$database_name = "cfec_database2"
$neighbor_node_table_name = "neighbor_nodes"
$c_value_table_name = "c_values"
$ip_node_table_name = "ip_nodes"
$now_time
$arr_ip = ["10.58.58.2", "10.58.58.3", "10.58.58.4", "10.58.58.5", "10.58.58.6", "10.58.58.7", "10.58.58.8", "10.58.58.9", "10.58.58.10", "10.58.58.11", "10.58.58.12", "10.58.58.13", "10.58.58.18", "10.58.58.20", "10.58.58.21", "10.58.58.22", "10.58.58.23", "10.58.58.24", "10.58.58.25", "10.58.58.26", "10.58.58.27", "10.58.58.28", "10.58.58.29", "10.58.58.98", "10.58.58.99", "10.58.58.100", "10.58.58.101", "10.58.58.102", "10.58.58.103", "10.58.58.104", "10.58.58.105", "10.58.58.106", "10.58.58.107", "10.58.58.108", "10.58.58.109", "10.58.58.34", "10.58.58.35", "10.58.58.36", "10.58.58.37", "10.58.58.38", "10.58.58.39", "10.58.58.40", "10.58.58.41", "10.58.58.42", "10.58.58.43", "10.58.58.44", "10.58.58.45", "10.58.58.82", "10.58.58.83", "10.58.58.84", "10.58.58.85", "10.58.58.86", "10.58.58.87", "10.58.58.88", "10.58.58.89", "10.58.58.90", "10.58.58.91", "10.58.58.92", "10.58.58.93", "10.58.58.114", "10.58.58.115", "10.58.58.116", "10.58.58.117", "10.58.58.118"]
$hash_node = {}
$hash_server_to_node = {}
$hash_server_to_serverdegree = {}
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
    if !$hash_server_to_node.has_key?(server_id) then
      $hash_server_to_node[server_id] = []
    end
    $hash_server_to_node[server_id].push(self)
  end
  def arr_neighbor_node()
    return @arr_neighbor_node
  end
  def oneHopPush(other_id)
    if !(@arr_neighbor_node.include?(other_id)) then
      @arr_neighbor_node.push(other_id)
    end
  end
  def degree()
    return @arr_neighbor_node.length
  end
end


def load_nodeid_round_robin()
  puts $network_filename
  ($hash_node.sort).each_with_index {|id_node_pair, i|
    id_node_pair[1].setServerId(i % $server_num)
  }
  $hash_server_to_node.each {|server_node_pair|
    server_node_pair[1].each {|node|
      if !$hash_server_to_serverdegree.has_key?(server_node_pair[0]) then
        $hash_server_to_serverdegree[server_node_pair[0]] = 0
      end
      $hash_server_to_serverdegree[server_node_pair[0]] += node.arr_neighbor_node.size()
    }
  }
  $hash_server_to_serverdegree.sort_by{ |_, v| v }.each {|server_degree|
    puts "#{$arr_ip[server_degree[0]]},#{server_degree[1]}"
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

#start()
#delete_column_from_neighbor_node_table()
#make_database()
#make_tables()
load_node()
load_nodeid_round_robin()
#p $hash_node[0]
#p $hash_node[0].arr_neighbor_node
