//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2007-2010, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "boost/python.hpp"

#include <sstream>

#include "boost/python/list.hpp"
#include "boost/python/tuple.hpp"
#include "boost/python/dict.hpp"
#include "boost/python/make_constructor.hpp"
#include "boost/python/call_method.hpp"

#include "IECore/CompoundData.h"
#include "IECore/bindings/RunTimeTypedBinding.h"

using std::string;
using namespace boost;
using namespace boost::python;

// Module declaration

namespace IECore
{

class CompoundDataFunctions
{
	public:
	
		IE_CORE_DECLAREMEMBERPTR( CompoundData );
		typedef const char * key_type;
		typedef CompoundDataMap::value_type::second_type data_type;
		typedef CompoundDataMap::size_type size_type;
		typedef CompoundDataMap::iterator iterator;
		typedef CompoundDataMap::const_iterator const_iterator;

		/// constructor that receives a python map object
		/// \todo Create a rvalue-from-python converter to replace this
		static CompoundDataPtr dataMapConstructor( dict v )
		{
			CompoundDataPtr mapPtr = new CompoundData();

			list values = v.values();
			list keys = v.keys();

			CompoundDataMap &newMap = mapPtr->writable();

			for ( int i = 0; i < keys.attr( "__len__" )(); i++ )
			{
				object key( keys[i] );
				object value( values[i] );
				extract<key_type> keyElem( key );
				extract<data_type> valueElem( value );
				if ( keyElem.check() )
				{
					//  try if elem is an exact data_type type
					if ( valueElem.check() )
					{
						newMap[keyElem()] = valueElem();
					}
					else
					{
						PyErr_SetString( PyExc_TypeError, "Incompatible data type." );
						throw_error_already_set();
					}
				}
				else
				{
					PyErr_SetString( PyExc_TypeError, "Incompatible key type. Only strings accepted." );
					throw_error_already_set();
				}
			}
			return mapPtr;
		}

		/// binding for __getitem__ function
		static data_type getItem( CompoundData &x, PyObject *i )
		{
			key_type key = convertKey( x, i );
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator value = xData.find( key );
			if ( value != xData.end() )
			{
				return value->second;
			}
			else
			{
				PyErr_SetString( PyExc_KeyError, key );
				throw_error_already_set();
			}
			return data_type();
		}

		/// binding for __setitem__ function
		static void setItem( CompoundData &x, PyObject *i, data_type v )
		{
			key_type key = convertKey( x, i );
			CompoundDataMap &xData = x.writable();
			xData[key] = v;
		}

		/// binding for __delitem__ function
		static void delItem( CompoundData &x, PyObject *i )
		{
			key_type key = convertKey( x, i );
			CompoundDataMap &xData = x.writable();
			CompoundDataMap::iterator value = xData.find( key );
			if ( value != xData.end() )
			{
				xData.erase( value );
			}
			else
			{
				PyErr_SetString( PyExc_KeyError, key );
				throw_error_already_set();
			}
		}

		/// binding for __len__ function
		static size_type len( CompoundData &x )
		{
			return x.readable().size();
		}

		/// binding for any unsupported binary operator
		CompoundDataPtr invalidOperator( CompoundData &x, PyObject* y )
		{
			PyErr_SetString( PyExc_SyntaxError, "Binary operator not supported for this class." );
			throw_error_already_set();
			assert( false );
			return 0;
		}

		/// binding for map clear method
		static void
		clear( CompoundData &x )
		{
			CompoundDataMap &xData = x.writable();
			xData.clear();
		}

		/// binding for has_key method
		static bool
		has_key( CompoundData &x, PyObject *i )
		{
			key_type key = convertKey( x, i );
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator value = xData.find( key );
			return ( value != xData.end() );
		}

		/// binding for items method
		static list
		items( CompoundData &x )
		{
			list newList;
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator iterX = xData.begin();
			while ( iterX != xData.end() )
			{
				newList.append( boost::python::make_tuple( iterX->first.value(), iterX->second ) );
				iterX++;
			}
			return newList;
		}

		/// binding for keys method
		static list
		keys( CompoundData &x )
		{
			list newList;
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator iterX = xData.begin();
			while ( iterX != xData.end() )
			{
				newList.append( iterX->first.value() );
				iterX++;
			}
			return newList;
		}

		/// binding for update method
		static void
		update1( CompoundData &x, CompoundData &y )
		{
			CompoundDataMap &xData = x.writable();
			const CompoundDataMap &yData = y.readable();
			CompoundDataMap::const_iterator iterY = yData.begin();

			for ( ; iterY != yData.end(); iterY++ )
			{
				xData[iterY->first] = iterY->second;
			}
		}

		/// binding for update method
		/// \todo This can be removed once we have a dict->CompoundData from-python converter
		static void
		update2( CompoundData &x, dict v )
		{
			list values = v.values();
			list keys = v.keys();

			CompoundDataMap &xData = x.writable();

			for ( int i = 0; i < keys.attr( "__len__" )(); i++ )
			{
				object key( keys[i] );
				object value( values[i] );
				extract<key_type> keyElem( key );
				extract<data_type> valueElem( value );
				if ( keyElem.check() )
				{
					//  try if elem is an exact data_type type
					if ( valueElem.check() )
					{
						xData[keyElem()] = valueElem();
					}
					else
					{
						PyErr_SetString( PyExc_TypeError, "Incompatible data type." );
						throw_error_already_set();
					}
				}
				else
				{
					PyErr_SetString( PyExc_TypeError, "Incompatible key type. Only strings accepted." );
					throw_error_already_set();
				}
			}
		}

		/// binding for values method
		static list
		values( CompoundData &x )
		{
			list newList;
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator iterX = xData.begin();
			while ( iterX != xData.end() )
			{
				newList.append( iterX->second );
				iterX++;
			}
			return newList;
		}

		/// binding for get method
		static data_type
		get( CompoundData &x, PyObject *i, PyObject *v )
		{
			key_type key = convertKey( x, i );
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator value = xData.find( key );
			if ( value == xData.end() )
			{
				extract<data_type> elem( v );
				if ( elem.check() )
				{
					return elem();
				}
				else
				{
					PyErr_SetString( PyExc_TypeError, "Invalid parameter" );
					throw_error_already_set();
				}
				return data_type();
			}
			// return the value from the map
			return value->second;
		}


		/// binding for setdefault method
		static data_type
		setdefault2( CompoundData &x, PyObject *i )
		{
			return setdefault( x, i, Py_None );
		}

		/// binding for setdefault method
		static data_type
		setdefault( CompoundData &x, PyObject *i, PyObject *v )
		{
			key_type key = convertKey( x, i );
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator value = xData.find( key );
			if ( value == xData.end() )
			{
				// the key is not there...
				if ( v == Py_None )
				{
					PyErr_SetString( PyExc_KeyError, key );
					throw_error_already_set();
				}
				// get the default value from the parameter
				extract<data_type> elem( v );
				if ( elem.check() )
				{
					// include the value on the map
					CompoundDataMap &writableX = x.writable();
					writableX[key] = elem();
					return elem();
				}
				else
				{
					PyErr_SetString( PyExc_TypeError, "Invalid parameter" );
					throw_error_already_set();
				}
				return data_type();
			}
			// return the value from the map.
			return value->second;
		}

		/// binding for pop method
		static data_type
		pop2( CompoundData &x, PyObject *i )
		{
			return pop( x, i, Py_None );
		}

		/// binding for pop method
		static data_type
		pop( CompoundData &x, PyObject *i, PyObject *v )
		{
			key_type key = convertKey( x, i );
			const CompoundDataMap &xData = x.readable();
			CompoundDataMap::const_iterator value = xData.find( key );
			if ( value == xData.end() )
			{
				if ( v == Py_None )
				{
					PyErr_SetString( PyExc_KeyError, key );
					throw_error_already_set();
				}
				extract<data_type> elem( v );
				if ( elem.check() )
				{
					return elem();
				}
				else
				{
					PyErr_SetString( PyExc_TypeError, "Invalid parameter" );
					throw_error_already_set();
				}
				return data_type();
			}
			// save the returning value.
			data_type ret( value->second );
			// delete it from the map
			CompoundDataMap &writableX = x.writable();
			CompoundDataMap::iterator writableValue = writableX.find( key );
			writableX.erase( writableValue );
			return ret;
		}

		/// binding for popitem method
		static boost::python::tuple
		popitem( CompoundData &x )
		{
			boost::python::tuple newTuple;
			CompoundDataMap &xData = x.writable();
			CompoundDataMap::iterator iterX = xData.begin();
			if ( iterX != xData.end() )
			{
				newTuple = boost::python::make_tuple( iterX->first.value(), iterX->second );
				xData.erase( iterX );
			}
			else
			{
				PyErr_SetString( PyExc_KeyError, "CompoundData is empty" );
				throw_error_already_set();
			}
			return newTuple;
		}

		/// \todo Move outside template so that it specialises the repr() in IECoreBinding.h
		static std::string repr( CompoundData &x )
		{
			std::stringstream s;

			s << "IECore." << x.typeName() << "(";

			bool added = false;
			for (
				CompoundDataMap::const_iterator it = x.readable().begin();
				it != x.readable().end();
				++it )
			{
				const std::string &key = it->first;

				object item( it->second );

				if ( item.attr( "__repr__" ) != object() )
				{
					std::string v = call_method< std::string >( item.ptr(), "__repr__" );

					if ( !added )
					{
						added = true;
						s << "{";
					}
					else
					{
						s << ",";
					}

					s << "'";
					s << key;
					s << "':";
					s << v;
				}
			}

			if ( added )
			{
				s << "}";
			}

			s << ")";

			return s.str();
		}

	protected:
		/*
		 * Utility functions
		 */

		static key_type
		convertKey( CompoundData & container, PyObject *key_ )
		{
			extract<key_type> key( key_ );
			if ( key.check() )
			{
				return key();
			}
			PyErr_SetString( PyExc_TypeError, "Invalid key type" );
			throw_error_already_set();
			return key_type();
		}

};

void bindCompoundData()
{
	RunTimeTypedClass<CompoundDataBase>();

	RunTimeTypedClass<CompoundData>(
		"This class behaves like the native python dict, except that it only accepts objects derived from Data class.\n"
		"The copy constructor accepts another instance of this class or a python dict containing Data objects\n"
		"it has the most important dict methods: has_key, items, keys, values, get, pop, etc.\n"
		)
		.def( init<>() )
		.def( "__init__", make_constructor( &CompoundDataFunctions::dataMapConstructor ), "Copy constructor: accepts a python dict containing Data objects." )
		.def( "__getitem__", &CompoundDataFunctions::getItem, "indexing operator.\nAccepts only string keys." )
		.def( "__setitem__", &CompoundDataFunctions::setItem, "index assignment operator.\nWorks exactly like on python dicts but only accepts Data objects as the new value." )
		.def( "__delitem__", &CompoundDataFunctions::delItem, "index deletion operator.\nWorks exactly like on python dicts." )
		.def( "__len__", &CompoundDataFunctions::len, "Length operator." )
		.def( "__contains__", &CompoundDataFunctions::has_key, "In operator.\nWorks exactly like on python dicts." )
		.def( "size", &CompoundDataFunctions::len, "m.size()\nReturns the number of elements on m. Same result as the len operator." )
		.def( "__cmp__", &CompoundDataFunctions::invalidOperator, "Raises an exception. CompoundData does not support comparison operators." )
		.def( "__repr__", &CompoundDataFunctions::repr )
		// python map methods.
		.def( "clear", &CompoundDataFunctions::clear, "m.clear()\nRemoves all items from m." )
		.def( "has_key", &CompoundDataFunctions::has_key, "m.has_key(k)\nReturns True if m has key k; otherwise, returns False." )
		.def( "items", &CompoundDataFunctions::items, "m.items()\nReturns a list of (key, value) pairs." )
		.def( "keys", &CompoundDataFunctions::keys, "m.keys()\nReturns a list of key values." )
		.def( "update", &CompoundDataFunctions::update1, "m.update(b)\nAdds all objects from b to m. b can be a CompoundData or a python dict." )
		.def( "update", &CompoundDataFunctions::update2 )
		.def( "values", &CompoundDataFunctions::values, "m.values()\nReturns a list of all values in m." )
		.def( "get", &CompoundDataFunctions::get, "m.get(k [, v])\nReturns m[k] if found; otherwise, returns v.",
			(
				boost::python::arg( "self" ),
				boost::python::arg( "key" ),
				boost::python::arg( "defaultValue" ) = object()
			)
		)
		.def( "setdefault", &CompoundDataFunctions::setdefault, "m.setdefault(k [, v])\nReturns m[k] if found; otherwise, returns v and sets m[k] = v." )
		.def( "setdefault", &CompoundDataFunctions::setdefault2 )
		.def( "pop", &CompoundDataFunctions::pop, "m.pop(k [,default])\nReturns m[k] if found and removes it from m; otherwise, returns default if supplied or raises KeyError if not." )
		.def( "pop", &CompoundDataFunctions::pop2 )
		.def( "popitem", &CompoundDataFunctions::popitem, "m.popitem()\nRemvoes a random (key,value) pair from m and returns it as a tuple." )
	;

}

} // namespace IECore
