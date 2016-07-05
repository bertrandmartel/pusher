/**********************************************************************************
 * This file is part of Pusher.                                                    *
 * <p/>                                                                            *
 * Copyright (C) 2016  Bertrand Martel                                             *
 * <p/>                                                                            *
 * Pusher is free software: you can redistribute it and/or modify                  *
 * it under the terms of the GNU General Public License as published by            *
 * the Free Software Foundation, either version 3 of the License, or               *
 * (at your option) any later version.                                             *
 * <p/>                                                                            *
 * Pusher is distributed in the hope that it will be useful,                       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                  *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                   *
 * GNU General Public License for more details.                                    *
 * <p/>                                                                            *
 * You should have received a copy of the GNU General Public License               *
 * along with Pusher. If not, see <http://www.gnu.org/licenses/>.                  *
 */
package com.github.akinaru.roboticbuttonpusher.constant;

/**
 * @author Bertrand Martel
 */
public class DefaultKeys {

    public final static byte[] AES_DEFAULT_KEY = new byte[]{
            (byte) 0xF2, (byte) 0x1E, (byte) 0x07, (byte) 0x8C, (byte) 0x96, (byte) 0x99, (byte) 0x5E, (byte) 0xF7,
            (byte) 0xED, (byte) 0xF0, (byte) 0x91, (byte) 0x84, (byte) 0x06, (byte) 0x06, (byte) 0xF3, (byte) 0x94,
            (byte) 0x59, (byte) 0x90, (byte) 0x66, (byte) 0x63, (byte) 0x81, (byte) 0xE9, (byte) 0x14, (byte) 0x3E,
            (byte) 0x7B, (byte) 0x02, (byte) 0x7E, (byte) 0x08, (byte) 0xB6, (byte) 0xC7, (byte) 0x06, (byte) 0x26
    };

    public final static byte[] IV_DEFAULT = new byte[]{
            (byte) 0xC3, (byte) 0x78, (byte) 0x7E, (byte) 0x76, (byte) 0x31, (byte) 0x6D, (byte) 0x6B, (byte) 0x5B,
            (byte) 0xB8, (byte) 0x8E, (byte) 0xDA, (byte) 0x03, (byte) 0x82, (byte) 0xEB, (byte) 0x57, (byte) 0xBD
    };

}
