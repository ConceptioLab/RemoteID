from setuptools import find_packages, setup

package_name = 'test_ros'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='Guilherme Matos de Oliveira',
    maintainer_email='guilhermematos@ita.br',
    description='Testing ros and vr_force',
    license='Apache License 2.0',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
                'talker = test_ros.publisher_member_function:main',
                'listener = test_ros.subscriber_member_function:main',
        ],
    },
)
