#ifndef MYAPP_AUTH_MODULE_HPP
#define MYAPP_AUTH_MODULE_HPP

namespace vix
{
  class App;
}

namespace myapp::auth
{
  class AuthModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace myapp::auth

#endif // MYAPP_AUTH_MODULE_HPP
