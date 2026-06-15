require 'benchmark'                                                              
require 'open3'                                                                  
stdout = stderr = status = nil                                                   
time = Benchmark.measure { stdout, stderr, status = Open3.capture3(*ARGV) }.total
$stdout.puts(stdout)                                                             
$stdout.puts(%{[link command: #{ARGV.join(' ')}]})                               
$stdout.puts(%{[LINK_TIME: #{time}]})                                            
$stderr.puts(stderr)                                                             
exit(status.success?)                                                            
