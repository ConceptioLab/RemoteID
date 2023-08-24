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
        
        coordenadas = data.get("coordinates", {})  # Obtém o dicionário de coordenadas ou um dicionário vazio se não existir
        msg.latitude = float(coordenadas.get("latitude", 0)) 
        msg.longitude = float(coordenadas.get("longitude", 0))
        msg.altitude = float(coordenadas.get("altitude", 0))
        msg.speed_horizontal = float(coordenadas.get("speed_horizontal", 0))
        msg.height_type = int(coordenadas.get("height_type", 0))
        msg.direction = float(coordenadas.get("direction", 0))
        msg.horiz_accuracy = int(coordenadas.get("horizAccuracy", 0))
        msg.vert_accuracy = int(coordenadas.get("vertAccuracy", 0))
        msg.speed_accuracy = int(coordenadas.get("speedAccuracy", 0))
        msg.speed_vertical = float(coordenadas.get("speed_vertical", 0))
        msg.altitudegeo = float(coordenadas.get("altitudegeo", 0))
        msg.altitudebaro = float(coordenadas.get("altitudebaro", 0))
        msg.height = float(coordenadas.get("height", 0))
            
        self.publisher_.publish(msg)
        self.get_logger().info('Publishing JSON: "%s"' % msg.uas_id)
        
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
