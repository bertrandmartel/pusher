/**********************************************************************************
 * This file is part of Button Pusher.                                             *
 * <p/>                                                                            *
 * Copyright (C) 2016  Bertrand Martel                                             *
 * <p/>                                                                            *
 * Button Pusher is free software: you can redistribute it and/or modify           *
 * it under the terms of the GNU General Public License as published by            *
 * the Free Software Foundation, either version 3 of the License, or               *
 * (at your option) any later version.                                             *
 * <p/>                                                                            *
 * Button Pusher is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   *
 * GNU General Public License for more details.                                    *
 * <p/>                                                                            *
 * You should have received a copy of the GNU General Public License               *
 * along with Button Pusher. If not, see <http://www.gnu.org/licenses/>.           *
 */
/**
	command.h
	list of commands
	@author Bertrand Martel
	@version 1.0
*/
E(COMMAND_GET_TOKEN            , 0x00)
E(COMMAND_ASSOCIATION_STATUS   , 0x01)
E(COMMAND_PUSH                 , 0x02)
E(COMMAND_SET_PASSWORD         , 0x03)
E(COMMAND_ASSOCIATE            , 0x04)
E(COMMAND_SET_KEY              , 0x05)
E(COMMAND_FAILURE              , 0x06)
E(COMMAND_ASSOCIATE_RESPONSE   , 0x07)
E(COMMAND_RECEIVE_KEYS         , 0x08)
E(COMMAND_SET_PASSWORD_RESPONSE, 0x09)
E(COMMAND_SET_KEYS_RESPONSE    , 0x0A)
E(COMMAND_DISASSOCIATION       , 0x0B)
E(COMMAND_MESSAGE_TOP          , 0x0C)
E(COMMAND_MESSAGE_BOTTOM       , 0x0D)
#undef E