/*
 * ebusd - daemon for communication with eBUS heating systems.
 * Copyright (C) 2014-2015 John Baier <ebusd@ebusd.eu>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEBUS_FILEREADER_H_
#define LIBEBUS_FILEREADER_H_

#include "symbol.h"
#include "result.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>

/** \file filereader.h */

using namespace std;

/** the separator character used between fields. */
#define FIELD_SEPARATOR ','

/** the separator character used to quote text having the @a FIELD_SEPARATOR in it. */
#define TEXT_SEPARATOR '"'

/** the separator character as string used to quote text having the @a FIELD_SEPARATOR in it. */
#define TEXT_SEPARATOR_STR "\""

extern void printErrorPos(ostream& out, vector<string>::iterator begin, const vector<string>::iterator end, vector<string>::iterator pos, string filename, size_t lineNo, result_t result);

extern unsigned int parseInt(const char* str, int base, const unsigned int minValue, const unsigned int maxValue, result_t& result, unsigned int* length);

/**
 * An abstract class that support reading definitions from a file.
 */
template<typename T>
class FileReader
{
public:

	/**
	 * Construct a new instance.
	 */
	FileReader(bool supportsDefaults)
			: m_supportsDefaults(supportsDefaults) {}

	/**
	 * Destructor.
	 */
	virtual ~FileReader() {}

	/**
	 * Read the definitions from a file.
	 * @param filename the name of the file being read.
	 * @param arg an argument to pass to @a addFromFile().
	 * @param verbose whether to verbosely log problems.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t readFromFile(const string filename, T arg=NULL, bool verbose=false)
	{
		ifstream ifs;
		ifs.open(filename.c_str(), ifstream::in);
		if (!ifs.is_open()) {
			m_lastError = filename;
			return RESULT_ERR_NOTFOUND;
		}
		string line;
		size_t lastSep = filename.find_last_of('/');
		size_t firstDot = filename.find_first_of('.', lastSep+1);
		string defaultDest = "";
		string defaultCircuit = "";
		if (lastSep!=string::npos && firstDot==lastSep+1+2) { // potential destination address, matches "^ZZ."
			result_t result;
			defaultDest = filename.substr(lastSep+1, 2);
			unsigned char zz = (unsigned char)parseInt(defaultDest.c_str(), 16, 0, 0xff, result, NULL);
			if (result!=RESULT_OK || !isValidAddress(zz))
				defaultDest = ""; // invalid: not in hex or no master/slave/broadcast address
			else {
				size_t endDot = filename.find_first_of('.', firstDot+1);
				if (endDot>firstDot && endDot-firstDot<=6) { // potential ident, matches "^ZZ.IDENT."
					defaultCircuit = filename.substr(firstDot+1, endDot-firstDot-1); // IDENT
					if (defaultCircuit.find_first_of(' ')!=string::npos)
						defaultCircuit = ""; // invalid: contains spaces
					else {
						size_t nextDot = filename.find_first_of('.', endDot+1);
						if (nextDot!=string::npos && nextDot>endDot+1) { // potential index suffix, matches "^ZZ.IDENT.[0-9]*."
							parseInt(filename.substr(endDot+1, nextDot-endDot-1).c_str(), 10, 1, 16, result, NULL);
							if (result==RESULT_OK)
								defaultCircuit = filename.substr(firstDot+1, nextDot-firstDot-1); // IDENT.[0-9]*
						}
					}
				}
			}
		}
		unsigned int lineNo = 0;
		vector<string> row;
		vector< vector<string> > defaults;
		while (getline(ifs, line) != 0) {
			lineNo++;
			splitFields(line, row);
			if (row.empty())
				continue;

			result_t result;
			vector<string>::iterator it = row.begin();
			const vector<string>::iterator end = row.end();
			if (m_supportsDefaults) {
				if (line[0] == '*') {
					row[0] = row[0].substr(1);
					result = addDefaultFromFile(defaults, row, it, defaultDest, defaultCircuit, filename, lineNo);
					if (result == RESULT_OK)
						continue;
				} else
					result = addFromFile(it, end, arg, &defaults, filename, lineNo);
			}
			else
				result = addFromFile(it, end, arg, NULL, filename, lineNo);

			if (result != RESULT_OK) {
				if (!verbose) {
					ifs.close();
					ostringstream error;
					error << filename << ":" << static_cast<unsigned>(lineNo);
					if (m_lastError.length()>0) {
						error << ": " << m_lastError;
					}
					m_lastError = error.str();
					return result;
				}
				if (m_lastError.length()>0) {
					cout << m_lastError << endl;
				}
				printErrorPos(cout, row.begin(), end, it, filename, lineNo, result);
			} else if (!verbose)
				m_lastError = "";
		}

		ifs.close();
		return RESULT_OK;
	}

	/**
	 * Return a @a string describing the last error position.
	 * @return a @a string describing the last error position.
	 */
	virtual string getLastError() { return m_lastError; }

	/**
	 * Add a default row that was read from a file.
	 * @param defaults the list to add the default row to.
	 * @param row the default row (initial star char removed).
	 * @param begin an iterator to the first column of the default row to read (for error reporting).
	 * @param defaultDest the valid destination address extracted from the file name (from ZZ part), or empty.
	 * @param defaultCircuit the valid circuit name extracted from the file name (from IDENT part), or empty.
	 * @param filename the name of the file being read.
	 * @param lineNo the current line number in the file being read.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t addDefaultFromFile(vector< vector<string> >& defaults, vector<string>& row,
			vector<string>::iterator& begin, string defaultDest, string defaultCircuit,
			const string& filename, unsigned int lineNo) {
		if (row.size()>1 && defaultCircuit.length()>0) {
			if (row[1].length()==0)
				row[1] = defaultCircuit; // set default circuit
			else if (row[1][0]=='#')
				row[1] = defaultCircuit+row[1]; // append security suffix to circuit prefix
		}
		if (row.size()>5 && defaultDest.length()>0 && row[5].length()==0)
			row[5] = defaultDest; // set default destination
		defaults.push_back(row);
		begin = row.end();
		return RESULT_OK;
	}

	/**
	 * Add a definition that was read from a file.
	 * @param begin an iterator to the first column of the definition row to read.
	 * @param end the end iterator of the definition row to read.
	 * @param arg the argument passed to @a readFromFile().
	 * @param defaults all previously read default rows (initial star char removed), or NULL if not supported.
	 * @param filename the name of the file being read.
	 * @param lineNo the current line number in the file being read.
	 * @return @a RESULT_OK on success, or an error code.
	 */
	virtual result_t addFromFile(vector<string>::iterator& begin, const vector<string>::iterator end,
		T arg, vector< vector<string> >* defaults,
		const string& filename, unsigned int lineNo) = 0;

	/**
	 * Left and right trim the string.
	 * @param str the @a string to trim.
	 */
	static void trim(string& str) {
		size_t pos = str.find_first_not_of(" \t");
		if (pos!=string::npos) {
			str.erase(0, pos);
		}
		pos = str.find_last_not_of(" \t");
		if (pos!=string::npos) {
			str.erase(pos+1);
		}
	}

	/**
	 * Split the line into fields.
	 * @param line the @a string with the line to split.
	 * @param row the @a vector to which to add the fields.
	 * @return true if the line was split, false if the line was completely empty or a comment line.
	 */
	static bool splitFields(string& line, vector<string>& row) {
		row.clear();
		trim(line);
		// skip empty lines and comments
		size_t length = line.length();
		if (length == 0 || line[0] == '#' || (line.length() > 1 && line[0] == '/' && line[1] == '/'))
			return false;

		bool quotedText = false, wasQuoted = false;
		ostringstream field;
		char prev = FIELD_SEPARATOR;
		for (size_t pos = 0; pos < length; pos++) {
			char ch = line[pos];
			switch (ch)
			{
			case FIELD_SEPARATOR:
				if (quotedText) {
					field << ch;
				} else {
					string str = field.str();
					trim(str);
					row.push_back(str);
					field.str("");
					wasQuoted = false;
				}
				break;
			case TEXT_SEPARATOR:
				if (prev == TEXT_SEPARATOR && !quotedText) { // double dquote
					field << ch;
					quotedText = true;
				} else if (quotedText) {
					quotedText = false;
				} else if (prev == FIELD_SEPARATOR) {
					quotedText = wasQuoted = true;
				} else {
					field << ch;
				}
				break;
			case '\r':
				break;
			default:
				if (prev==TEXT_SEPARATOR && !quotedText && wasQuoted) {
					field << TEXT_SEPARATOR; // single dquote in the middle of formerly quoted text
					quotedText = true;
				}
				field << ch;
				break;
			}
			prev = ch;
		}
		string str = field.str();
		trim(str);
		row.push_back(str);
		return true;
	}

private:

	/** whether this instance supports rows with defaults (starting with a star). */
	bool m_supportsDefaults;

protected:

	/** a @a string describing the last error position. */
	string m_lastError;

};

#endif // LIBEBUS_FILEREADER_H_
