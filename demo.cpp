#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <typeinfo>
#include <sstream>
#include <list>

#include <boost/make_shared.hpp>
#include <boost/fusion/adapted/std_tuple.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/list.hpp>

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/vector_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/plus.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/inserter.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/reverse.hpp>

#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/include/algorithm.hpp>



namespace mpl = boost::mpl;

struct print_type {
	template <class T>
	void operator () (T) const {
		std::cout << typeid(T).name() << std::endl;
	}
};


template <class TSeq>
void print_types() {
	mpl::for_each<TSeq>(print_type());
	std::cout << std::endl;
}

typedef std::pair<int,int> Coord;
typedef std::list<Coord> Position;


template<typename Archive>
struct saver {
   saver(Archive& ar):ar(ar) {}

   template<typename T>
   void operator()(T& o) const {
	  ar & o;
   }
   Archive& ar;
};

template<typename Archive, typename sequence>
struct LoaderType {
	sequence& tt;
	Archive& ar;
	LoaderType(Archive& ar, sequence& tt):ar(ar), tt(tt) {}

	template<typename T>
	void operator()() {
		T& t = boost::fusion::find<T>(tt);
		ar & t;
	}

};


template<typename archive, typename sequence>
void generic_save(archive& ar, const sequence* v) {
   boost::fusion::for_each(*v, saver<archive>(ar));
}


//////////////////////////////////////////////////////////// {{{
template<typename ...Args>
struct my_class{};

template<typename ...Args>
struct Constructor{
	template<typename T>
	static void construct(T* that, Args...args) {
		return std::_Construct(that, args...);
	}
};

/*
struct use_default{};
template<typename T0 = use_default,
typename T1 = use_default>
struct my_class{};
*/
/*
template<> struct my_class<> {};
template<typename T> struct my_class<T>{};
template<typename T,typename U> struct my_class<T,U>{};
*/

namespace impl{

template<typename F,typename L>
struct exit : boost::mpl::equal_to<
   typename boost::mpl::distance<F,L>::type,
   boost::mpl::int_<0>
>{};

template<typename F,typename L, bool exit,
typename ...Args>
struct to_variadic{
   typedef typename boost::mpl::deref<F>::type front_;
   typedef typename boost::mpl::next<F>::type next_;
   typedef typename impl::exit<next_,L>::type exit_;
   typedef typename to_variadic<next_,L,
    exit_::value,front_,Args...>::type type;
};

template<typename F,typename L,typename ...Args>
struct to_variadic<F,L,true,Args...>{
   typedef Constructor<Args...> type;
};

template<typename Seq>
struct seq_traits{
   typedef typename boost::mpl::begin<Seq>::type first_;
   typedef typename boost::mpl::end<Seq>::type last_;
   typedef typename impl::exit<first_,last_>::type exit_;
};

}//impl

template<typename Seq>
struct to_variadic{
   typedef typename boost::mpl::reverse<
    Seq>::type reversed_;
   typedef typename impl::to_variadic<
      typename impl::seq_traits<reversed_>::first_,
      typename impl::seq_traits<reversed_>::last_,
      impl::seq_traits<Seq>::exit_::value
   >::type type;
};




//////////////////////////////////////////////////////////// }}}








class Node {
	public:
		Node(
			unsigned int number_,
			std::string name_,
			Coord coord_
		):
		number(number_),
		name(name_),
		position(1,coord_){}

		std::string repr() const {
			std::stringstream ss;
			auto coord = position.front();
			ss << "Number: " << number << "; ";
			ss << "Name: " << name << "; ";
			ss << "Coord: " << coord.first << ", " << coord.second << "; ";
			return ss.str();
		}
	private:
		unsigned int number;
		std::string name;
		Position position;
		friend boost::serialization::access;
		friend boost::fusion::extension::access;
		BOOST_SERIALIZATION_SPLIT_MEMBER()
		template<typename Archive>
			void save(Archive& ar, const unsigned int) const {
				generic_save(ar,this);
			}
		template<typename Archive>
			void load(Archive& ar, const unsigned int) {}
};

BOOST_FUSION_ADAPT_STRUCT(
	Node, 
	(unsigned int, number)
	(std::string, name)
	(Position, position)
);


template < class T, class R >
struct ToStdTuple;

template < class... TTypes, class X >
struct ToStdTuple< std::tuple< TTypes... >, X >
{
  typedef std::tuple< TTypes..., X > type;
};


template<typename T>
struct MplSeqToStdTuple {
	   typedef typename mpl::copy<
		   T,
		   mpl::back_inserter<mpl::vector<>>
		   >::type result;
	typedef typename mpl::fold< result, std::tuple<>, ToStdTuple< mpl::_1, mpl::_2 > >::type type;
};



namespace boost { namespace serialization {

template<typename Archive, typename Class, typename Member>
struct _LoaderSpec {
	void load(Archive& ar, Member& t) const {
		ar & t;
	}
};

//template<typename Archive>
//struct _LoaderSpec<Archive, Node, Position> {
	
//}

template<typename Class>
struct ConstructProxy {
	template<typename T>
	auto operator()(T& t) -> decltype(t) {
		return t;
	}
};

/*
template<>
struct ConstructProxy<Node> {
	template<typename T>
	auto operator()(T& t) -> decltype(t)
	{
		return t;
	}

	auto operator()(Position& t) -> decltype(t.front())
	{
		return t.front();
	}
};
*/

#define DECL_CONSTRUCTOR_PROXY(Class, F) \
template<> \
struct ConstructProxy<Class> { \
	template<typename T> \
	auto operator()(T& t) -> decltype(t) \
	{ \
		return t; \
	} \
	F \
}

DECL_CONSTRUCTOR_PROXY(Node,
	Coord operator()(Position& t) {
		return t.front();
	}
);


template<typename Archive>
struct _Loader {
	Archive& ar;
	_Loader(Archive& ar_):ar(ar_) {
	}

	template<typename T>
	T& operator()(T& t) const {
		ar & t;
		std::cout << "Load:" << typeid(Coord).name() << std::endl;
		return t;
		//return _LoaderSpec<Archive,Class,T>().load();
	}
};

template<int ...>
struct seq { };

template<int N, int ...S>
struct gens : gens<N-1, N-1, S...> { };

template<int ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};


template<class T, typename Archive>
class ObjectMaker {
public:
    ObjectMaker(T* that_, Archive& ar) : 
		that(that_),
		loader(ar)
    {
    }
	
	void make() {
		_Load();
		_Construct();
	}
private:
	void _Load() {
		boost::fusion::for_each(m_ctorArgs, loader);
	}

    void _Construct() {
		const int TSize = boost::mpl::size<T>::type::value;
        CallF( typename gens<TSize>::type() );
    }
	
	typedef typename MplSeqToStdTuple<T>::type TType;

    template< int ...S >
    void CallF( seq<S...>) {
		ConstructProxy<T> constructProxy;
		std::_Construct(that, constructProxy(std::get<S>(m_ctorArgs)) ...);
    }
	T* that;
	_Loader<Archive> loader;
    TType m_ctorArgs;
};




//template<typename T, typename Archive>
//struct Loader {

	//T* that;
	//_Loader<Archive> loader;
	//Loader(T* that_, Archive& ar_):that(that_),loader(ar_) {}

	//template<typename ...Args>
	//void constr(Args&& ...args) {
		//std::_Construct(that, loader(args) ...);
		////::new(static_cast<void*>(that)) T(args...);
	//}
//};

template<typename Archive, typename sequence>
void generic_load(Archive& ar, sequence* v) {
		ObjectMaker<sequence,Archive> m(v,ar);
		m.make();
}

template<typename Archive>
	void load_construct_data(Archive& ar, Node* that, const unsigned int) {
		generic_load(ar,that);
	}
}}//boost::serialization

typedef boost::shared_ptr<Node> NodePtr;

void save_node(NodePtr n, const char * filename){
    std::ofstream ofs(filename);
    boost::archive::text_oarchive oa(ofs);
	oa.register_type<Node>();
    oa << n;
}

NodePtr load_node(const char * filename)
{
	NodePtr n;
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);
	ia.register_type<Node>();
    ia >> n;
	return n;
}

template<typename T>
struct TConstructor {
	   typedef typename mpl::copy<
		   T,
		   mpl::back_inserter<mpl::vector<>>
		   >::type result;
       typedef typename to_variadic<result>::type type;
};
void test_mpl_to_variadic() {
   typedef boost::mpl::int_<0>  _0;
   typedef boost::mpl::int_<1>  _1;

   {
       typedef to_variadic<
        boost::mpl::vector<> >::type found_;
       typedef Constructor<>  wanted_;
       BOOST_MPL_ASSERT((boost::is_same<found_,wanted_>));
   }
   {
       typedef to_variadic<
        boost::mpl::vector<_0> >::type found_;
       typedef Constructor<_0> wanted_;
       BOOST_MPL_ASSERT((boost::is_same<found_,wanted_>));
   }
   {
       typedef to_variadic<
        boost::mpl::vector<_1,_0> >::type found_;
       typedef Constructor<_1,_0> wanted_;
       BOOST_MPL_ASSERT((boost::is_same<found_,wanted_>));
   }
   {
	   //typedef mpl::copy<
		   //Node,
		   //mpl::back_inserter<mpl::vector<>>
		   //>::type result;
       //typedef to_variadic<result>::type NodeConstructor;
	   typedef TConstructor<Node>::type found_;
       typedef Constructor<unsigned int, std::string, Position> wanted_;
	   BOOST_MPL_ASSERT((boost::is_same<found_,wanted_>));
   } 
   {
	   //typedef mpl::vector<unsigned int , std::string, int> VTuple;
	   //VTuple t;
   }

}

int main() {
	//typedef mpl::range_c<int,0,10> numbers;
	//const int glast = mpl::back<numbers>::type::value;
	//std::cout << glast << std::endl;
	using mpl::_;
	
	typedef 
		mpl::vector<
			mpl::vector_c<int,1,2,3>,
			mpl::vector_c<int,4,5,6>,
			mpl::vector_c<int,7,8,9>
		> seq;


	typedef
		mpl::transform
		<
			seq,
			mpl::at<_, mpl::int_<2> >,
			mpl::inserter
			<
				mpl::int_<0>,
				mpl::plus<_,_>
			>
		>::type sum;

	//std::cout << sum::value << std::endl;

	typedef 
		mpl::vector<int,double,float>
		someSeq;
	
	//print_types<seq>();
	NodePtr node1 = boost::make_shared<Node>(10,"Help",std::make_pair(20, 30));
	save_node(node1, "C:/temp/out.txt");
	NodePtr node2 = load_node("C:/temp/out.txt");
	std::cout << node1->repr() << std::endl;
	std::cout << node2->repr() << std::endl;
	//std::cout << boost::fusion::at_c<0>(node) << std::endl;
	//typedef boost::mpl::at<Node,mpl::int_<1>>::type asdf;
	//asdf x = "10";
	//std::cout << x << std::endl;


	return 0;
}


#if 0
/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// demo.cpp
//
// (C) Copyright 2002-4 Robert Ramey - http://www.rrsd.com .
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <cstddef> // NULL
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>

#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

/////////////////////////////////////////////////////////////
// The intent of this program is to serve as a tutorial for
// users of the serialization package.  An attempt has been made
// to illustrate most of the facilities of the package.  
//
// The intent is to create an example suffciently complete to
// illustrate the usage and utility of the package while
// including a minimum of other code. 
//
// This illustration models the bus system of a small city.
// This includes, multiple bus stops,  bus routes and schedules.
// There are different kinds of stops.  Bus stops in general will
// will appear on multiple routes.  A schedule will include
// muliple trips on the same route.

/////////////////////////////////////////////////////////////
// gps coordinate
//
// llustrates serialization for a simple type
//
class gps_position
{
    friend std::ostream & operator<<(std::ostream &os, const gps_position &gp);
    friend class boost::serialization::access;
    int degrees;
    int minutes;
    float seconds;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int /* file_version */){
        ar & degrees & minutes & seconds;
    }
public:
    // every serializable class needs a constructor
    gps_position(){};
    gps_position(int _d, int _m, float _s) : 
        degrees(_d), minutes(_m), seconds(_s)
    {}
};
std::ostream & operator<<(std::ostream &os, const gps_position &gp)
{
    return os << ' ' << gp.degrees << (unsigned char)186 << gp.minutes << '\'' << gp.seconds << '"';
}

/////////////////////////////////////////////////////////////
// One bus stop
//
// illustrates serialization of serializable members
//

class bus_stop
{
    friend class boost::serialization::access;
    friend std::ostream & operator<<(std::ostream &os, const bus_stop &gp);
    virtual std::string description() const = 0;
    gps_position latitude;
    gps_position longitude;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & latitude;
        ar & longitude;
    }
protected:
    bus_stop(const gps_position & _lat, const gps_position & _long) :
        latitude(_lat), longitude(_long)
    {}
public:
    bus_stop(){}
    virtual ~bus_stop(){}
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(bus_stop)

std::ostream & operator<<(std::ostream &os, const bus_stop &bs)
{
    return os << bs.latitude << bs.longitude << ' ' << bs.description();
}

/////////////////////////////////////////////////////////////
// Several kinds of bus stops
//
// illustrates serialization of derived types
//
class bus_stop_corner : public bus_stop
{
    friend class boost::serialization::access;
    std::string street1;
    std::string street2;
    virtual std::string description() const
    {
        return street1 + " and " + street2;
    }
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        // save/load base class information
        ar & boost::serialization::base_object<bus_stop>(*this);
        ar & street1 & street2;
    }

public:
    bus_stop_corner(){}
    bus_stop_corner(const gps_position & _lat, const gps_position & _long,
        const std::string & _s1, const std::string & _s2
    ) :
        bus_stop(_lat, _long), street1(_s1), street2(_s2)
    {
    }
};

class bus_stop_destination : public bus_stop
{
    friend class boost::serialization::access;
    std::string name;
    virtual std::string description() const
    {
        return name;
    }
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & boost::serialization::base_object<bus_stop>(*this) & name;
    }
public:
    
    bus_stop_destination(){}
    bus_stop_destination(
        const gps_position & _lat, const gps_position & _long, const std::string & _name
    ) :
        bus_stop(_lat, _long), name(_name)
    {
    }
};

/////////////////////////////////////////////////////////////
// a bus route is a collection of bus stops
//
// illustrates serialization of STL collection templates.
//
// illustrates serialzation of polymorphic pointer (bus stop *);
//
// illustrates storage and recovery of shared pointers is correct
// and efficient.  That is objects pointed to by more than one
// pointer are stored only once.  In such cases only one such
// object is restored and pointers are restored to point to it
//
class bus_route
{
    friend class boost::serialization::access;
    friend std::ostream & operator<<(std::ostream &os, const bus_route &br);
    typedef bus_stop * bus_stop_pointer;
    std::list<bus_stop_pointer> stops;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        // in this program, these classes are never serialized directly but rather
        // through a pointer to the base class bus_stop. So we need a way to be
        // sure that the archive contains information about these derived classes.
        //ar.template register_type<bus_stop_corner>();
        ar.register_type(static_cast<bus_stop_corner *>(NULL));
		ar.register_type<bus_stop_corner>();
        //ar.template register_type<bus_stop_destination>();
        ar.register_type(static_cast<bus_stop_destination *>(NULL));
        // serialization of stl collections is already defined
        // in the header
        ar & stops;
    }
public:
    bus_route(){}
    void append(bus_stop *_bs)
    {
        stops.insert(stops.end(), _bs);
    }
};
std::ostream & operator<<(std::ostream &os, const bus_route &br)
{
    std::list<bus_stop *>::const_iterator it;
    // note: we're displaying the pointer to permit verification
    // that duplicated pointers are properly restored.
    for(it = br.stops.begin(); it != br.stops.end(); it++){
        os << '\n' << std::hex << "0x" << *it << std::dec << ' ' << **it;
    }
    return os;
}

/////////////////////////////////////////////////////////////
// a bus schedule is a collection of routes each with a starting time
//
// Illustrates serialization of STL objects(pair) in a non-intrusive way.
// See definition of operator<< <pair<F, S> >(ar, pair) and others in
// serialization.hpp
// 
// illustrates nesting of serializable classes
//
// illustrates use of version number to automatically grandfather older
// versions of the same class.

class bus_schedule
{
public:
    // note: this structure was made public. because the friend declarations
    // didn't seem to work as expected.
    struct trip_info
    {
        template<class Archive>
        void serialize(Archive &ar, const unsigned int file_version)
        {
            // in versions 2 or later
            if(file_version >= 2)
                // read the drivers name
                ar & driver;
            // all versions have the follwing info
            ar & hour & minute;
        }

        // starting time
        int hour;
        int minute;
        // only after system shipped was the driver's name added to the class
        std::string driver;

        trip_info(){}
        trip_info(int _h, int _m, const std::string &_d) :
            hour(_h), minute(_m), driver(_d)
        {}
    };
private:
    friend class boost::serialization::access;
    friend std::ostream & operator<<(std::ostream &os, const bus_schedule &bs);
    friend std::ostream & operator<<(std::ostream &os, const bus_schedule::trip_info &ti);
    std::list<std::pair<trip_info, bus_route *> > schedule;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int version)
    {
        ar & schedule;
    }
public:
    void append(const std::string &_d, int _h, int _m, bus_route *_br)
    {
        schedule.insert(schedule.end(), std::make_pair(trip_info(_h, _m, _d), _br));
    }
    bus_schedule(){}
};
BOOST_CLASS_VERSION(bus_schedule::trip_info, 2)

std::ostream & operator<<(std::ostream &os, const bus_schedule::trip_info &ti)
{
    return os << '\n' << ti.hour << ':' << ti.minute << ' ' << ti.driver << ' ';
}
std::ostream & operator<<(std::ostream &os, const bus_schedule &bs)
{
    std::list<std::pair<bus_schedule::trip_info, bus_route *> >::const_iterator it;
    for(it = bs.schedule.begin(); it != bs.schedule.end(); it++){
        os << it->first << *(it->second);
    }
    return os;
}

void save_schedule(const bus_schedule &s, const char * filename){
    // make an archive
    std::ofstream ofs(filename);
    boost::archive::text_oarchive oa(ofs);
    oa << s;
}

void
restore_schedule(bus_schedule &s, const char * filename)
{
    // open the archive
    std::ifstream ifs(filename);
    boost::archive::text_iarchive ia(ifs);

    // restore the schedule from the archive
    ia >> s;
}

int main(int argc, char *argv[])
{   
    // make the schedule
    bus_schedule original_schedule;

    // fill in the data
    // make a few stops
    bus_stop *bs0 = new bus_stop_corner(
        gps_position(34, 135, 52.560f),
        gps_position(134, 22, 78.30f),
        "24th Street", "10th Avenue"
    );
    bus_stop *bs1 = new bus_stop_corner(
        gps_position(35, 137, 23.456f),
        gps_position(133, 35, 54.12f),
        "State street", "Cathedral Vista Lane"
    );
    bus_stop *bs2 = new bus_stop_destination(
        gps_position(35, 136, 15.456f),
        gps_position(133, 32, 15.300f),
        "White House"
    );
    bus_stop *bs3 = new bus_stop_destination(
        gps_position(35, 134, 48.789f),
        gps_position(133, 32, 16.230f),
        "Lincoln Memorial"
    );

    // make a  routes
    bus_route route0;
    route0.append(bs0);
    route0.append(bs1);
    route0.append(bs2);

    // add trips to schedule
    original_schedule.append("bob", 6, 24, &route0);
    original_schedule.append("bob", 9, 57, &route0);
    original_schedule.append("alice", 11, 02, &route0);

    // make aother routes
    bus_route route1;
    route1.append(bs3);
    route1.append(bs2);
    route1.append(bs1);

    // add trips to schedule
    original_schedule.append("ted", 7, 17, &route1);
    original_schedule.append("ted", 9, 38, &route1);
    original_schedule.append("alice", 11, 47, &route1);

    // display the complete schedule
    std::cout << "original schedule";
    std::cout << original_schedule;
    
    std::string filename(boost::archive::tmpdir());
    filename += "/demofile.txt";

    // save the schedule
    save_schedule(original_schedule, filename.c_str());

    // ... some time later
    // make  a new schedule
    bus_schedule new_schedule;

    restore_schedule(new_schedule, filename.c_str());

    // and display
    std::cout << "\nrestored schedule";
    std::cout << new_schedule;
    // should be the same as the old one. (except for the pointer values)

    delete bs0;
    delete bs1;
    delete bs2;
    delete bs3;
    return 0;
}

#endif//


