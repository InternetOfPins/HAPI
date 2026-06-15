require 'benchmark'                                                                                      
require 'erb'                                                                                            
require 'fileutils'                                                                                      
require 'open3'                                                                                          
require 'pathname'                                                                                       
require 'time'                                                                                           
                                                                                                         
                                                                                                         
def report_error(command_line, stdout, stderr, code)                                                     
  raise [%{\ncommand line: #{command_line}},                                                            
         %{stdout\n#{'-'*80}\n#{stdout}},                                                              
         %{stderr\n#{'-'*80}\n#{stderr}},                                                              
         %{code\n#{'-'*80}\n#{code}}].join(%{\n\n})                                                  
end                                                                                                      
                                                                                                         
# Build the specified CMake target and return the measurements that were taken.                          
# The exact format of a measurement returned by this function is explained                               
# in the JavaScript code that loads it, in the chart template file below.                                
def build(target)                                                                                        
  command = ['/usr/bin/cmake', '--build', '/home/azevedo/Work/r-site.net/C++/IOP/HAPI/benchmark/build', '--target', target]                   
  exe_file = %{/home/azevedo/Work/r-site.net/C++/IOP/HAPI/benchmark/build/_metabench/#{target}}                                     
  cpp_file = %{/home/azevedo/Work/r-site.net/C++/IOP/HAPI/benchmark/build/_metabench/#{target}.cpp}                                                           
                                                                                                         
  # We change the timestamp of the source file and remove the executable                                 
  # to make sure CMake considers the target as outdated; otherwise, we                                   
  # might skip the compilation and/or link steps. This is because CMake's                                
  # timestamps are not precise enough.                                                                   
  FileUtils.touch(cpp_file, mtime: Time.now+1)                                                           
  File.delete(exe_file) if File.exist?(exe_file)                                                         
                                                                                                         
  stdout, stderr, status = Open3.capture3(*command)                                                      
  compile_cli = stdout.match(/\[compilation command: (.+)\]/i)                                         
  compile_cli = compile_cli ? compile_cli.captures[0] : '(unavailable)'                                  
  link_cli = stdout.match(/\[link command: (.+)\]/i)                                                   
  link_cli = link_cli ? link_cli.captures[0] : '(unavailable)'                                           
                                                                                                         
  if not status.success?                                                                                 
    cli = %{compile: #{compile_cli}\nlink: #{link_cli}}                                                 
    report_error(cli, stdout, stderr, IO.read(cpp_file))                                                 
  end                                                                                                    
                                                                                                         
  result = {}                                                                                            
                                                                                                         
  # Compilation and link times in seconds. They are output to stdout because                             
  # we use the `compile.rb` and `link.rb` scripts below to launch the compiler                           
  # and linker with CMake.                                                                               
  result['COMPILATION_TIME'] = stdout.match(/\[COMPILATION_TIME: (.+)\]/i).captures[0].to_f            
  result['LINK_TIME'] = stdout.match(/\[LINK_TIME: (.+)\]/i).captures[0].to_f                          
                                                                                                         
  # Peak memory usage                                                                                    
  result['PEAK_MEMORY'] = stdout.match(/\[PEAK_MEMORY: (.+)\]/i).captures[0].to_f                      
                                                                                                         
  # Size of the generated executable in KB                                                               
  result['EXECUTABLE_SIZE'] = File.size(exe_file).to_f / 1000                                            
                                                                                                         
  return result                                                                                          
end                                                                                                      
                                                                                                         
# Render the ERB template and return the generated code.                                                 
def render(erb_template, n, env)                                                                         
  begin                                                                                                  
    ERB.new(File.read(erb_template)).result(binding)                                                     
  rescue Exception => e                                                                                  
    $stderr.puts(%{\nError while generating a C++ file from the ERB template #{erb_template}:\n})      
    raise e                                                                                              
  end                                                                                                    
end                                                                                                      
                                                                                                         
# Formats the progress bar that we print while we run the benchmark                                      
def progress_bar(range, index, start_time, filename)                                                     
   n = range[index]                                                                                      
   percentage = (index+1) * 100 / range.size                                                             
   relative = filename.relative_path_from(Pathname.getwd)                                                
   elapsed = Time.now - start_time                                                                       
   return %{==> #{percentage}% (#{elapsed.round(2)}s) #{relative} (n = #{n})}                            
end                                                                                                      
                                                                                                         
# Returns an array of measurements representing the compilation of an ERB                                
# template for the values of `n` in the specified `range`.                                               
def measure(target, erb_template, range, env, repetitions)                                               
  erb_template = Pathname.new(erb_template)                                                              
  cpp_file = %{/home/azevedo/Work/r-site.net/C++/IOP/HAPI/benchmark/build/_metabench/#{target}.cpp}                                                           
  data = []                                                                                              
  range = range.to_a                                                                                     
  start = Time.now                                                                                       
  $stderr.write(progress_bar(range, 0, start, erb_template)) # Setup the initial progress bar            
  range.each_with_index do |n, index|                                                                    
    compile = -> (code) {                                                                                
      IO.write(cpp_file, code)                                                                           
      return repetitions.times.map { build(target) }                                                     
    }                                                                                                    
    code = render(erb_template, n, env)                                                                  
    compile[code] if index == 0 # Fill the cache on the first iteration                                  
    base = compile[code]                                                                                 
    total = compile[%{#define METABENCH\n} + code]                                                      
    data << {'n' => n, 'base' => base, 'total' => total}                                                 
    $stderr.write(%{\r} + progress_bar(range, index, start, erb_template)) # Update the progress bar    
  end                                                                                                    
  return data                                                                                            
ensure                                                                                                   
  $stderr.puts # Otherwise the output of the next CMake command appears on the same line                 
  IO.write(cpp_file, '')                                                                                 
end                                                                                                      
