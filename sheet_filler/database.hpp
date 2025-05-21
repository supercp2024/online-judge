#include <unordered_map>
#include <filesystem>
#include <fstream>

#include <mysql_driver.h>
#include <cppconn/driver.h>
#include <mysql_connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <jsoncpp/json/json.h>

class sql_client
{
public:
    sql_client();
    ~sql_client();
    void setInput(const std::string& name, std::string str);
    void setOutput(const std::string& name, std::string str);
    void setCode(const std::string& name, std::filesystem::path path);
    void setLimit(const std::string& name, std::unordered_map<unsigned int, unsigned int>&& limits);
    void setInfo(const std::string& name, const std::string& difficulty, const std::string& tags, std::string str);
private:
    sql::Driver* _driver_ptr;
    sql::Connection* _con_ptr;
};

enum class resource_limit : unsigned int
{
    /* Per-process CPU limit, in seconds.  */
    CPU = 0,
    /* Largest file that can be created, in bytes.  */
    FSIZE = 1, 
    /* Maximum size of data segment, in bytes.  */
    DATA = 2,
    /* Maximum size of stack segment, in bytes.  */
    STACK = 3, 
    /* Largest core file that can be created, in bytes.  */
    CORE = 4,
    /* Largest resident set size, in bytes.
       This affects swapping; processes that are exceeding their
       resident set size will be more likely to have physical memory
       taken from them.  */
    RSS = 5,
    /* Number of processes.  */
    NPROC = 6,
    /* Number of open files.  */
    NOFILE = 7,
    /* BSD name for same.  */
    OFILE = NOFILE,
    /* Locked-in-memory address space.  */
    MEMLOCK = 8,
    /* Address space limit.  */
    AS = 9,
    /* Maximum number of file locks.  */
    LOCKS = 10,
    /* Maximum number of pending signals.  */
    SIGPENDING = 11,
    /* Maximum bytes in POSIX message queues.  */
    MSGQUEUE = 12,
    /* Maximum nice priority allowed to raise to.
       Nice levels 19 .. -20 correspond to 0 .. 39
       values of this resource limit.  */
    NICE = 13, 
    /* Maximum realtime priority allowed for non-priviledged
       processes.  */
    RTPRIO = 14, 
    /* Maximum CPU time in microseconds that a process scheduled under a real-time
       scheduling policy may consume without making a blocking system
       call before being forcibly descheduled.  */
    RTTIME = 15, 
    NLIMITS = 16,
};