import rclpy
from rclpy.node import Node
from std_msgs.msg import String

class Processor(Node):
    _x = 0
    _y = 0

    def __init__(self):
        super().__init__('processor')
        self.subscription_ = self.create_subscription(String, '/publisher', self.callback, 10)
        self.publisher_ = self.create_publisher(String, '/processor', 10)
        self.timer = self.create_timer(0.5, self.publish_message)
        self.subscription_

    def callback(self, msg):
        self.get_logger().info(f'Received WASD: "{msg.data}"')
        if msg.data == 'w':
            self._y += 1
        elif msg.data == 'a':
            self._x-= 1
        elif msg.data == 's':
            self._y -= 1
        elif msg.data == 'd':
            self._x += 1

    def publish_message(self):
        msg = String()
        msg.data = f'({self._x}, {self._y})'
        self.publisher_.publish(msg)
        self.get_logger().info(f'Publishing Position: "({self._x}, {self._y})"')

def main():
    rclpy.init()
    node = Processor()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
