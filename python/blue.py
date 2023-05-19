import os
import sys
import uuid
import dbus
import dbus.exceptions
import dbus.mainloop.glib
import array
from gi.repository import GLib

def advertisement_sent_callback():
    print("Advertisement sent!")

def advertisement_error_callback(error):
    print("Advertisement error:", str(error))

def start_advertisement():
    bus = dbus.SystemBus()
    manager = dbus.Interface(bus.get_object('org.bluez', '/org/bluez/hci0'), 'org.bluez.LEAdvertisingManager1')
    adapter = dbus.Interface(bus.get_object('org.bluez', '/org/bluez/hci0'), 'org.bluez.Adapter1')

    try:
        mainloop = GLib.MainLoop()
        service_uuids = [uuid.UUID('0000110e-0000-1000-8000-00805f9b34fb')]  # Exemplo de UUID

        advertisement = dbus.Dictionary({
            'Type': 'peripheral',
            'ServiceUUIDs': service_uuids,
            'IncludeTxPower': True,
            'LocalName': 'Hello'
        })

        options = dbus.Dictionary({'Discoverable': True, 'Connectable': True})

        manager.RegisterAdvertisement(advertisement, options, reply_handler=advertisement_sent_callback, error_handler=advertisement_error_callback)
        mainloop.run()

    except KeyboardInterrupt:
        print("Advertisement stopped")
        manager.UnregisterAdvertisement(service)

def stop_advertisement():
    bus = dbus.SystemBus()
    manager = dbus.Interface(bus.get_object('org.bluez', '/org/bluez/hci0'), 'org.bluez.LEAdvertisingManager1')
    manager.UnregisterAdvertisement(service)
    mainloop.quit()

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    try:
        start_advertisement()
    except dbus.exceptions.DBusException as e:
        print(str(e))
        sys.exit(1)






