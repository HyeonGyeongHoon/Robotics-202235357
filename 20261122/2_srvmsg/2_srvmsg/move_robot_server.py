import rclpy
from rclpy.node import Node
from custom_interfaces.srv import MoveRobot

import math  # Use sin, cos function


class MoveRobotServer(Node):
    def __init__(self):
        super().__init__('move_robot_server')

        self.srv = self.create_service(MoveRobot, '/move_robot', self.srv_callback)  # wait request
        self.get_logger().info('Move Robot Server Ready.')

    def srv_callback(self, request, response):
        cur_x = request.current_pose.x  # get current x
        cur_y = request.current_pose.y  # get current y
        degree = request.current_pose.theta  # get current degree
        distance = request.distance  # get distance

        # To use sin, cos function, change Degree to Radian
        degree_to_radian = math.radians(degree)

        # next position = current position + weighted distance
        next_x = cur_x + math.cos(degree_to_radian) * distance
        next_y = cur_y + math.sin(degree_to_radian) * distance

        # set next position in response
        response.result_pose.x = next_x
        response.result_pose.y = next_y
        response.result_pose.theta = degree

        # Move Success!!
        response.success = True
        
        self.get_logger().info(f'Incoming request\ncurrent x: {request.current_pose.x}\ncurrent y: {request.current_pose.y}\ntheta: {request.current_pose.theta}\ndistance: {request.distance}')
        self.get_logger().info(f'Sending back response\nnext x: {response.result_pose.x}\nnext y: {response.result_pose.y}\ntheta: {response.result_pose.theta}')
        
        return response


def main():
    rclpy.init()
    node = MoveRobotServer()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
