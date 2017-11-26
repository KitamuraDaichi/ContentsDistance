while true do
  date = `date`
  ifconfig = `ifconfig eth1 | grep RX | grep TX`
  ifconfig.gsub!("\t", "")
  ifconfig.gsub!(" ", "")
  ifconfig.gsub!("\n", "")
  File.open("./RXTX.csv", "a") do |f|
    f.puts "#{ifconfig},#{date}"
  end
  sleep(1)
end
