#!/usr/bin/env ruby

$network_filename = ARGV[0]
$node_num = (ARGV[1]).to_i
$output = ""
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

def output_file()
  output = ""
  node_counter = 0
  node_upper = $node_num + 2600
  output_hash_node = {}
  real_node_num = 0
  while real_node_num < $node_num do
    $output = ""
    $hash_node.each {|node_id, node|
      if node_counter >= node_upper then
        break
      else
        output_hash_node[node_id] = node
        node_counter += 1
      end
    }
    #puts "output_hash_node_length: #{output_hash_node.length}"
    #exit(0)
    test_hash_node = {}
    output_hash_node.sort.each {|node_id, node|
      node.arr_neighbor_node().uniq.sort.each {|neighbor_node_id|
        if output_hash_node.has_key?(neighbor_node_id) then
          $output << "#{node_id} #{neighbor_node_id}\n"
          test_hash_node[node_id] = true
          test_hash_node[neighbor_node_id] = true
        end
      }
    }
    real_node_num = test_hash_node.length
    puts "real node_num: #{real_node_num}"
    puts "node upper:    #{node_upper}"
    node_upper += 1
    node_counter = 0
  end
  File.open("./test_data_twittter_limit#{$node_num}_real#{real_node_num}node.txt", "w") do |file|
    file.puts($output)
  end
end

load_node()
output_file()

