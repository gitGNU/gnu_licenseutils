/*  Copyright (C) 2014 Ben Asselstine

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/
#ifndef LU_FSF_ADDRESSES_H
#define LU_FSF_ADDRESSES_H 1

char * get_address (int address, char *license, int num_spaces);

extern struct argp fsf_addresses_argp;
enum fsf_addresses_t
{
  FSF_ADDRESS_LINK = 0x01,
  FSF_ADDRESS_FRANKLIN = 0x02,
  FSF_ADDRESS_TEMPLE = 0x04,
  FSF_ADDRESS_MASS = 0x08,
};

#endif
