/***************************************************************************
 * This program is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *                                                                         *
 * Copyright (C) 2010 Kadek Wisnu Arsadhi kadek.wisnu@gmail.com            *
 *                                                                         *
 *                                                                         *
 * iso8583 extract - Extract bit fields of any ISO 8583 financial          *
 * based messages under GPL Licence                                        *
 *                                                                         *
 ***************************************************************************/
#ifndef INCLUDE_FLEXIBITY_ISOFIELD_HPP_
#define INCLUDE_FLEXIBITY_ISOFIELD_HPP_

#include <string>

enum class FieldFormat_e {
	FIELD_N = 0,	// Numeric values only
	FIELD_Z,		// Tracks 2 and 3 code set as defined in ISO/IEC 7813 and ISO/IEC 4909 respectively
	FIELD_ANP,		// Alphabetic, numeric and space character
	FIELD_ANS,		// Alphabetic, numeric and special characters
	FIELD_AN,		// Alphabetic and numeric
	FIELD_B,		// Binary data
	FIELD_BITMAP,	// Bitmap
};

struct IsoField
{
	IsoField() = default;
	IsoField(const IsoField&) = default;
	IsoField(unsigned long lengthType, const std::string & description, FieldFormat_e format ):
		lengthType(lengthType),description(description), format(format)
	{

	}

	unsigned long lengthType;
	std::string description;
	FieldFormat_e format;
	std::string value;
};

#endif // INCLUDE_FLEXIBITY_ISOFIELD_HPP_
