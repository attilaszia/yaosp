# yaosp bindings for python
#
# Copyright (c) 2009 Attila Magyar
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of version 2 of the GNU General Public License
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

import _yaosp as yo
import os
import errno

INFINITE_TIMEOUT = -1

class IpcError( yo.error ): pass

class IpcReceiveException( IpcError ):
	def __init__(self, errno):
		self.errno = errno
		self.message = 'IPC receive error: ' + os.strerror( errno )

	def __str__(self):
		return "%s (%d)" % ( self.message, self.errno )

class IpcTimeoutException( IpcReceiveException ):
	def __init__(self, msg = ""):
		IpcReceiveException.__init__( self, -errno.ETIME )
		if msg: self.message += '; ' + msg


class IpcMessageListener(object):
	def received( self, message, code, size ): print "Message %s; code %d" % ( message, code )
	def error( self, ipcReceiveException ): print ipcReceiveException

class IpcClient(object):

	def __init__( self, name="", port=-1 ):
		if not name and port < 0:
			raise ValueError( "No IPC port or name parameter is passed" )
		self.name = name
		self.port = port

	def  open( self ):
		if self.port < 0:
			self.port = yo.get_named_ipc( self.name )
			if self.port < 0:
				raise IpcError( "Can't query IPC by name %s" % ( self.name ) )
		return self.port

	def send( self, message, code = 0 ):
		if self.port < 0:
			raise IpcError( 'IPC %s is not opened' % ( self.name ) )

		return yo.send_ipc_message( self.port, code, message )

class IpcServer(object):

	def __init__(self, name = "", max_message_size = 32):
		self.port = -1
		self.name = name
		self.max_message_size = max_message_size
		self.stop = True
		self._listeners = []
		self._timeout = 1000

	def add_listener( self, listener ):
		if listener in self._listeners:
			return False
		self._listeners.append( listener )
		return True

	def delete_listener( self, listener ):
		return self._listeners.remove( listener )

	def clear_listeners( self ):
		self._listeners = []

	def open( self ):
		self.port = yo.create_ipc_port()
		if self.port < 0:
			raise IpcError( "Can't create ipc port: %d" % ( self.port ) )

		if self.name:
			ecode = yo.register_named_ipc( self.name, self.port )
			if ecode < 0:
				self.port = -1
				raise IpcError( "Can't not register ipc port: %s; err: %d" % ( self.name, ecode ) )

		return self.port

	def stop( self ):
		self.stop = True

	def read( self, timeout= INFINITE_TIMEOUT ):
		if self.port < 0:
			raise IpcError( 'IPC %s is not opened' % ( self.name ) )

		result, code, data = yo.recv_ipc_message( self.port, self.max_message_size, timeout )
		if result == -errno.ETIME:
			raise IpcTimeoutException( "IPC timeout: %d" % ( self.port ) )
		if result < 0:
			raise IpcReceiveException( result )

		return result, code, data

	def _notify_msg_listeners( self, message, code, size ):
		for listener in self._listeners:
			listener.received( message, code, size )

	def _notify_error_listeners( self, exception ):
		for listener in self._listeners:
			listener.error( exception )

	def start_loop( self ):
		self.stop = False
		self._event_loop()

	def _event_loop( self ):
		if self.port < 0:
			raise IpcError( 'IPC port is not opened' )

		while not self.stop:
			try:
				result, code, message = self.read( timeout=self._timeout )
			except IpcTimeoutException, e:
				pass # ignore
			except IpcReceiveException, e:
				self._notify_error_listeners( e )
			else:
				self._notify_msg_listeners( message, code, result )
