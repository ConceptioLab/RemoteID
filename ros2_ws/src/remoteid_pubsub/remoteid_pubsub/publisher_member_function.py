# Copyright 2016 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import rclpy
from rclpy.node import Node
import json

from conceptio_msgs.msg import Remoteid


class MinimalPublisher(Node):

    def __init__(self):
        super().__init__('minimal_publisher')
        self.publisher_ = self.create_publisher(Remoteid, 'conceptio/unit/air/simulation/remoteid', 10)
        timer_period = 0.5  # seconds
        self.timer = self.create_timer(timer_period, self.timer_callback)
        self.i = 0

    def timer_callback(self):
        data = self.read_json_file("/opt/conceptio/remote_id/uav.json") 
        msg = Remoteid()
        msg.uas_id = data["uas_id"]
        msg.ua_type = data["UAType"]
        msg.uas_caa_id = data["uas_caa_id"]
        msg.description = data["description"]
        msg.operator_id = data["operatorID"]
        if(data["coordinates"]["latitude"]) :
            msg.latitude = data["coordinates"]["latitude"]
        else:
            msg.latitude = 0
        if(data["coordinates"]["longitude"]) :
            msg.longitude = data["coordinates"]["longitude"]
        else:
            msg.longitude = 0
        if(data["coordinates"]["altitude"]) :
            msg.altitude = data["coordinates"]["altitude"]
        else:
            msg.altitude = 0
        if(data["coordinates"]["speed_horizontal"]) :
            msg.speed_horizontal = data["coordinates"]["speed_horizontal"]
        else:
            msg.speed_horizontal = 0
        if(data["coordinates"]["height_type"]) :
            msg.height_type = data["coordinates"]["height_type"]
        else:
            msg.height_type = 0
        if(data["coordinates"]["direction"]) :
            msg.direction = data["coordinates"]["direction"]
        else:
            msg.direction = 0
        if(data["coordinates"]["horizAccuracy"]) :
            msg.horizAccuracy = data["coordinates"]["horizAccuracy"]
        else:
            msg.horizAccuracy = 0
        if(data["coordinates"]["vertAccuracy"]) :
            msg.vertAccuracy = data["coordinates"]["vertAccuracy"]
        else:
            msg.vertAccuracy = 0
        if(data["coordinates"]["speedAccuracy"]) :
            msg.speedAccuracy = data["coordinates"]["speedAccuracy"]
        else:
            msg.speedAccuracy = 0
        if(data["coordinates"]["speed_vertical"]) :
            msg.speed_vertical = data["coordinates"]["speed_vertical"]
        else:
            msg.speed_vertical = 0
        if(data["coordinates"]["altitudegeo"]) :
            msg.altitudegeo = data["coordinates"]["altitudegeo"]
        else:
            msg.altitudegeo = 0
        if(data["coordinates"]["altitudebaro"]) :
            msg.altitudebaro = data["coordinates"]["altitudebaro"]
        else:
            msg.altitudebaro = 0
        if(data["coordinates"]["height"]) :
            msg.height = data["coordinates"]["height"]
        else:
            msg.height = 0
            
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing JSON: "%s"' % msg)
        
    def read_json_file(self, file_path):
        with open(file_path, 'r') as json_file:
            data = json.load(json_file)
            return data


def main(args=None):
    rclpy.init(args=args)

    minimal_publisher = MinimalPublisher()

    rclpy.spin(minimal_publisher)

    # Destroy the node explicitly
    # (optional - otherwise it will be done automatically
    # when the garbage collector destroys the node object)
    minimal_publisher.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
