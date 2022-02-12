#include <geometry_msgs/Twist.h>
#include <ros/ros.h>
#include <signal.h>
#include <std_msgs/Float64.h>
#include <stdio.h>
#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#define KEYCODE_RIGHT 0x43
#define KEYCODE_LEFT 0x44
#define KEYCODE_UP 0x41
#define KEYCODE_DOWN 0x42
#define KEYCODE_B 0x62
#define KEYCODE_C 0x63
#define KEYCODE_D 0x64
#define KEYCODE_E 0x65
#define KEYCODE_F 0x66
#define KEYCODE_G 0x67
#define KEYCODE_Q 0x71
#define KEYCODE_R 0x72
#define KEYCODE_T 0x74
#define KEYCODE_V 0x76
#define KEYCODE_P 0x70
#define KEYCODE_M 0x6D
#define KEYCODE_S 0x73

class KeyboardReader {
public:
  KeyboardReader()
#ifndef _WIN32
      : kfd(0)
#endif
  {
#ifndef _WIN32
    // get the console in raw mode
    tcgetattr(kfd, &cooked);
    struct termios raw;
    memcpy(&raw, &cooked, sizeof(struct termios));
    raw.c_lflag &= ~(ICANON | ECHO);
    // Setting a new line, then end of file
    raw.c_cc[VEOL] = 1;
    raw.c_cc[VEOF] = 2;
    tcsetattr(kfd, TCSANOW, &raw);
#endif
  }
  void readOne(char *c) {
#ifndef _WIN32
    int rc = read(kfd, c, 1);
    if (rc < 0) {
      throw std::runtime_error("read failed");
    }
#else
    for (;;) {
      HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
      INPUT_RECORD buffer;
      DWORD events;
      PeekConsoleInput(handle, &buffer, 1, &events);
      if (events > 0) {
        ReadConsoleInput(handle, &buffer, 1, &events);
        if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_LEFT) {
          *c = KEYCODE_LEFT;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_UP) {
          *c = KEYCODE_UP;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT) {
          *c = KEYCODE_RIGHT;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
          *c = KEYCODE_DOWN;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x42) {
          *c = KEYCODE_B;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x43) {
          *c = KEYCODE_C;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x44) {
          *c = KEYCODE_D;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x45) {
          *c = KEYCODE_E;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x46) {
          *c = KEYCODE_F;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x47) {
          *c = KEYCODE_G;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x51) {
          *c = KEYCODE_Q;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x52) {
          *c = KEYCODE_R;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x54) {
          *c = KEYCODE_T;
          return;
        } else if (buffer.Event.KeyEvent.wVirtualKeyCode == 0x56) {
          *c = KEYCODE_V;
          return;
        }
      }
    }
#endif
  }
  void shutdown() {
#ifndef _WIN32
    tcsetattr(kfd, TCSANOW, &cooked);
#endif
  }

private:
#ifndef _WIN32
  int kfd;
  struct termios cooked;
#endif
};

KeyboardReader input;

class TeleopTurtle {
public:
  TeleopTurtle();
  void keyLoop();

private:
  ros::NodeHandle nh_;
  double linear_, angular_, l_scale_, a_scale_;
  ros::Publisher twist_pub_, cmd_vel_lin_x_pub_, cmd_vel_ang_z_pub_;
};

TeleopTurtle::TeleopTurtle()
    : linear_(0), angular_(0), l_scale_(2.0), a_scale_(2.0) {
  nh_.param("scale_angular", a_scale_, a_scale_);
  nh_.param("scale_linear", l_scale_, l_scale_);

  // twist_pub_ = nh_.advertise<geometry_msgs::Twist>("cmd_vel", 1);
  cmd_vel_lin_x_pub_ = nh_.advertise<std_msgs::Float64>("cmd_vel_linear_x", 1);
  cmd_vel_ang_z_pub_ = nh_.advertise<std_msgs::Float64>("cmd_vel_angular_z", 1);
}

void quit(int sig) {
  (void)sig;
  input.shutdown();
  ros::shutdown();
  exit(0);
}

int main(int argc, char **argv) {
  ros::init(argc, argv, "clavier_node");
  TeleopTurtle teleop_turtle;

  signal(SIGINT, quit);

  teleop_turtle.keyLoop();
  quit(0);

  return (0);
}

void TeleopTurtle::keyLoop() {
  char c;
  bool dirty = false;

  puts("Reading from keyboard");
  puts("---------------------------");
  puts("Use arrow keys to move the turtle. 'q' to quit.");
  puts("'p' : augmenter vitesse lin/ang ++");
  puts("'m' : diminuer vitesse lin/ang --");
  // puts("'s' : stopper le robot");

  double vit_lin_save = 1.0;
  double vit_ang_save = 1.0;

  double vit_lin = 1.0;
  double vit_ang = 1.0;

  for (;;) {
    // get the next event from the keyboard
    try {
      input.readOne(&c);
    } catch (const std::runtime_error &) {
      perror("read():");
      return;
    }

    linear_ = angular_ = 0;
    ROS_DEBUG("value: 0x%02X\n", c);

    if (vit_lin == 0.0 || vit_ang == 0.0)
      vit_lin = vit_lin_save;
    vit_ang = vit_ang_save;

    switch (c) {
    case KEYCODE_LEFT:
      ROS_DEBUG("LEFT");
      angular_ = vit_ang;
      dirty = true;
      break;
    case KEYCODE_RIGHT:
      ROS_DEBUG("RIGHT");
      angular_ = -vit_ang;
      dirty = true;
      break;
    case KEYCODE_UP:
      ROS_DEBUG("UP");
      linear_ = vit_lin;
      dirty = true;
      break;
    case KEYCODE_DOWN:
      ROS_DEBUG("DOWN");
      linear_ = -vit_ang;
      dirty = true;
      break;
    case KEYCODE_P:
      ROS_DEBUG("PLUS");
      vit_lin += 0.2;
      vit_ang += 0.2;
      if (vit_lin > 3.0)
        vit_lin = 3.0;
      if (vit_ang > 3.0)
        vit_ang = 3.0;

      vit_ang_save = vit_ang;
      vit_lin_save = vit_lin;

      ROS_INFO("Vitesse lin/ang = %f", vit_lin);

      break;
    case KEYCODE_M:
      ROS_DEBUG("MOINS");
      vit_lin -= 0.2;
      vit_ang -= 0.2;
      if (vit_lin < 0)
        vit_lin = 0.2;
      if (vit_ang < 0)
        vit_ang = 0.2;

      vit_ang_save = vit_ang;
      vit_lin_save = vit_lin;

      ROS_INFO("Vitesse lin/ang = %f", vit_lin);

      break;

      /*case KEYCODE_S:
        ROS_DEBUG("STOP");
        vit_lin = 0.0;
        vit_ang = 0.0;

        ROS_INFO("STOP !");

        break;
       */

    case KEYCODE_Q:
      ROS_DEBUG("quit");
      return;
    }

    geometry_msgs::Twist twist;
    std_msgs::Float64 vel_x, ang_z;

    twist.angular.z = a_scale_ * angular_;
    twist.linear.x = l_scale_ * linear_;
    vel_x.data = twist.linear.x;
    ang_z.data = twist.angular.z;
    if (dirty == true) {
      // twist_pub_.publish(twist);
      cmd_vel_lin_x_pub_.publish(vel_x);
      cmd_vel_ang_z_pub_.publish(ang_z);
      dirty = false;
    }
  }

  return;
}
