from setuptools import setup

package_name = 'remoteid_pubsub'

setup(
    name=package_name,
    version='0.0.0',
    packages=[package_name],
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Guilherme Matos de Oliveira',
    maintainer_email='guilhermematos@ita.br',
    description='RemoteID node on ros2.',
    license='Apache License 2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
                'talker = remoteid_pubsub.publisher_member_function:main',
                'listener = remoteid_pubsub.subscriber_member_function:main',
        ],
    },
)