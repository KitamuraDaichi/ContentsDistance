column_num = `mysql -uroot cfec_database2 -e 'select count(*) from c_values'`
column_num.gsub!("count(*)\n", "")
puts "a: #{column_num}"
