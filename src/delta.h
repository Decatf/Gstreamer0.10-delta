/*
    Noise Sharpening dsp
    Copyright (C) 2010 Robert Y <Decatf@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

gint8 *process8 (void* buf, gint size, gint nch, gfloat gain);
guint8 *process8u (void* buf, gint size, gint nch, gfloat gain);
gint16 *process16 (void* buf, gint size, gint nch, gfloat gain);
guint16 *process16u (void* buf, gint size, gint nch, gfloat gain);
gint32 *process32 (void* buf, gint size, gint nch, gfloat gain);
guint32 *process32u (void* buf, gint size, gint nch, gfloat gain);
gint64 *process64 (void* buf, gint size, gint nch, gfloat gain);
guint64 *process64u (void* buf, gint size, gint nch, gfloat gain);
gfloat *processf (void* buf, gint size, gint nch, gfloat gain);

