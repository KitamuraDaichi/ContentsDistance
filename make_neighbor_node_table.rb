#!/usr/bin/env ruby
require "date"

# Catalogue Server ID
$server_number = 0
# ラウンドロビンでノードをサーバに分ける
$network_filename = ARGV[0]
$server_num = 3
# 作成したいデータベースとテーブルの名前
$database_name = "cfec_database"
$neighbor_node_table_name = "neighbor_nodes"
$c_value_table_name = "c_values"
$now_time

def make_database()
  command = "mysql -phige@mos -uroot -e 'create database #{$database_name}'"
  if !system(command) then
    STDERR.puts "データベースを作れませんでした。"
  end
end

def make_tables()
  command = "mysql -p -uroot -e 'create table #{$database_name}.#{$neighbor_node_table_name} (own_content_id varchar(33), other_content_id varchar(33), version_id varchar(25))'"
  if !system(command) then
    STDERR.puts "#{$neighbor_node_table_name}を作れませんでした。"
  end

  command = "mysql -p -uroot -e 'create table #{$database_name}.#{$c_value_table_name} (own_content_id varchar(33), other_content_id varchar(33), version_id varchar(25), value_chain varchar(257), path_chain varchar(199), recv_time_stamp timestamp)'"
  if !system(command) then
    STDERR.puts "#{$c_value_table_name}を作れませんでした。"
  end
end

def conv_fileid_to_catid(node)
  cat_server_id = ((node.to_i % $server_num).to_s(16)).rjust(8, "0") 
  node_index = (node.to_i.to_s(16)).rjust(8, "0")
  catid = cat_server_id + "00000000" + "00000000" + node_index

  return catid
end

def insert_neighbor_node_table(own_node, other_node)
  own_node_id = conv_fileid_to_catid(own_node)
  other_node_id = conv_fileid_to_catid(other_node)
  time_str = "#{$now_time.year}#{$now_time.month}#{($now_time.day.to_s).rjust(2, "0")}#{$now_time.hour}#{$now_time.min}"
  version_id = ($server_num.to_s(16)).rjust(8, "0") + time_str

  command = "mysql -phige@mos -uroot -e 'insert into #{$database_name}.#{$neighbor_node_table_name} (own_content_id, other_content_id, version_id) values (\"#{own_node_id}\", \"#{other_node_id}\", \"#{version_id}\")'" 
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

def load_nodeid_round_robin()
  puts $network_filename
  File.open($network_filename) do |file|
    file.each_line do |line|
      arr_edge = line.split(" ")
      if ((arr_edge[0].to_i % $server_num) == $server_number) then 
        insert_neighbor_node_table(arr_edge[0], arr_edge[1])
      else
      end
      if ((arr_edge[1].to_i % $server_num) == $server_number) then 
        insert_neighbor_node_table(arr_edge[1], arr_edge[0])
      else
      end
    end
  end
end

def start()
  $now_time = DateTime.now
end

start()
delete_column_from_neighbor_node_table()
#make_database()
#make_tables()
load_nodeid_round_robin()
