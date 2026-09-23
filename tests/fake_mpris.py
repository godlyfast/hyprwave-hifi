#!/usr/bin/env python3
"""Minimal fake MPRIS player for hyprwave selection tests.

Usage: fake_mpris.py <bus-name> <identity> <playback-status> <has-metadata>
Example: fake_mpris.py org.mpris.MediaPlayer2.vlc "VLC media player" Playing 1
"""
import sys

import dbus
import dbus.mainloop.glib
import dbus.service
from gi.repository import GLib

BUS_NAME, IDENTITY, STATUS, HAS_META = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4] == "1"

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()


class Mpris(dbus.service.Object):
    @dbus.service.method("org.freedesktop.DBus.Properties", in_signature="ss", out_signature="v")
    def Get(self, iface, prop):
        return self.GetAll(iface)[prop]

    @dbus.service.method("org.freedesktop.DBus.Properties", in_signature="s", out_signature="a{sv}")
    def GetAll(self, iface):
        if iface == "org.mpris.MediaPlayer2":
            return {"Identity": IDENTITY, "CanRaise": False, "CanQuit": False}
        if iface == "org.mpris.MediaPlayer2.Player":
            props = {
                "PlaybackStatus": STATUS, "CanSeek": True, "CanPlay": True,
                "CanGoNext": True, "CanGoPrevious": True, "CanPause": True,
                "Position": dbus.Int64(0), "Rate": 1.0, "Volume": 1.0,
                "Shuffle": False, "LoopStatus": "None",
            }
            if HAS_META:
                props["Metadata"] = dbus.Dictionary({
                    "mpris:trackid": dbus.ObjectPath("/fake/1"),
                    "mpris:length": dbus.Int64(200_000_000),
                    "xesam:title": "Test Track",
                    "xesam:artist": dbus.Array(["Test Artist"], signature="s"),
                }, signature="sv")
            return props
        return {}

    @dbus.service.method("org.mpris.MediaPlayer2.Player", in_signature="", out_signature="")
    def PlayPause(self):
        pass


# Keep a reference: an unreferenced BusName is garbage-collected and the
# name is released immediately, so the service would never appear on the bus.
_name = dbus.service.BusName(BUS_NAME, bus)
Mpris(bus, "/org/mpris/MediaPlayer2")
GLib.MainLoop().run()
