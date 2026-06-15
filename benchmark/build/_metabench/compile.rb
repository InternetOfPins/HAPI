require 'benchmark'                                                              
require 'open3'                                                                  
stdout = stderr = status = nil                                                   
command_line = ['/usr/bin/ruby', '--', '/home/azevedo/Work/r-site.net/C++/IOP/HAPI/benchmark/build/_metabench/memusg.rb'] + ARGV          
time = Benchmark.measure {                                                       
  stdout, stderr, status = Open3.capture3(*command_line)                         
}.total                                                                          
$stdout.puts(stdout)                                                             
$stdout.puts(%{[compilation command: #{command_line.join(' ')}]})                
$stdout.puts(%{[COMPILATION_TIME: #{time}]})                                     
$stderr.puts(stderr)                                                             
exit(status.success?)                                                            
