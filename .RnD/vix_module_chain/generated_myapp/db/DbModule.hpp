#ifndef MYAPP_DB_MODULE_HPP
#define MYAPP_DB_MODULE_HPP

namespace vix
{
  class App;
}

namespace myapp::db
{
  class DbModule
  {
  public:
    static const char *name();
    static void register_routes(vix::App &app);
  };
} // namespace myapp::db

#endif // MYAPP_DB_MODULE_HPP
