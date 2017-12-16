date = `date`
column_num = `mysql -uroot cfec_database2 -e 'select count(*) from c_values'`
column_num.gsub!("count(*)\n", "").gsub!("\n", "")
File.open("./c_values_column_num.csv", "a") do |f|
  f.puts "#{column_num},#{date}"
end
i = 0
count = 0
while true do
  date = `date`
  ifconfig = `ifconfig eth1 | grep RX | grep TX`
  ifconfig.gsub!("\t", "")
  ifconfig.gsub!(" ", "")
  ifconfig.gsub!("\n", "")
  File.open("./RXTX.csv", "a") do |f|
    f.puts "#{ifconfig},#{date}"
  end
  date = `date`
  mem = `ps aux | grep condis | grep -v grep`
  mem.gsub!("\t", ",")
  mem.gsub!("    ", ",")
  mem.gsub!("   ", ",")
  mem.gsub!("  ", ",")
  mem.gsub!(" ", ",")
  mem.gsub!("\n", "")
  File.open("./mem.csv", "a") do |f|
    f.puts "#{mem},#{date}"
  end
  if i >= (59 * 5) then
  #if i >= 3 then
    column_num = `mysql -uroot cfec_database2 -e 'select count(*) from c_values'`
    column_num.gsub!("count(*)\n", "").gsub!("\n", "")
    File.open("./c_values_column_num.csv", "a") do |f|
      f.puts "#{column_num},#{date}"
    end
    i = 0
  end
  i += 1
  sleep(1)
end
