module OS                                                                        
  def OS.windows?                                                                
    (/cygwin|mswin|mingw|bccwin|wince|emx/ =~ RUBY_PLATFORM) != nil              
  end                                                                            
  def OS.mac?                                                                    
   (/darwin/ =~ RUBY_PLATFORM) != nil                                            
  end                                                                            
  def OS.unix?                                                                   
    !OS.windows?                                                                 
  end                                                                            
  def OS.linux?                                                                  
    OS.unix? and not OS.mac?                                                     
  end                                                                            
end                                                                              
                                                                                 
if OS.mac?                                                                       
  def memusg(pgid)                                                               
    `/bin/ps -o rss= -g #{pgid}`.to_i                                            
  end                                                                            
elsif OS.linux?                                                                  
  def memusg(pgid)                                                               
    `/bin/ps -o rss= -#{pgid}`.to_i                                              
  end                                                                            
else                                                                             
  throw %{Unsupported platform #{RUBY_PLATFORM}}                                 
end                                                                              
                                                                                 
pid = Process.spawn(*ARGV)                                                       
Process.detach(pid) # Make sure the child process does not become a zombie       
peak = 0                                                                         
# Loop until getpgid throws ESRCH, which means the process is not alive anymore  
begin                                                                            
  while true                                                                     
    pgid = Process.getpgid(pid)                                                  
    peak = [peak, memusg(pgid)].max                                              
    sleep 0.01                                                                   
  end                                                                            
rescue Errno::ESRCH                                                              
end                                                                              
                                                                                 
puts %{[PEAK_MEMORY: #{peak}]}                                                   
