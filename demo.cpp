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
#include <boost/mpl/replace.hpp>

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

template<typename Class>
struct SaverProxy {
	template<typename T>
	auto operator()(T& t) -> decltype(t) const {
		return t;
	}
};


template<typename Archive, typename Class>
struct saver {
   saver(Archive& ar):ar(ar){}

   template<typename T>
   void operator()(T& o) const {
	   auto value(sproxy(o));
	   ar & value;
   }
   Archive& ar;
   SaverProxy<Class> sproxy;
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
   boost::fusion::for_each(*v, saver<archive, sequence>(ar));
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


class SomeSinglton {
	SomeSinglton(){}
	~SomeSinglton(){}
	SomeSinglton(const SomeSinglton&){}
	SomeSinglton& operator=(const SomeSinglton&) {return *this;}
	public:
	static SomeSinglton* instance() {
		static SomeSinglton s;
		return &s;
	}
	
	std::string repr() const {
		return "SomeSinglton";
	}

};

struct SomeSingltonID {
	std::string name;

	template<typename Archive>
	void serialize(Archive& ar, const unsigned int) {
		ar & name;
	}
};

SomeSingltonID getNameOfSomeSinglton(const SomeSinglton* that) {
	return SomeSingltonID{std::string("SomeSinglton")};
}


SomeSinglton* getSomeSingltonByName(const SomeSingltonID& name) {
	SomeSinglton* that =  SomeSinglton::instance();
	assert(name.name == getNameOfSomeSinglton(that).name);
	return that;
}




class Node {
	public:
		Node(
			const SomeSinglton& traits_,
			unsigned int number_,
			std::string name_,
			const Coord& coord_
		):
		traits(traits_),
		number(number_),
		name(name_),
		position(1,coord_){}

		std::string repr() const {
			std::stringstream ss;
			auto coord = position.front();
			ss << "Number: " << number << "; ";
			ss << "Name: " << name << "; ";
			ss << "Coord: " << coord.first << ", " << coord.second << "; ";
			ss << "Singlton: " << traits.repr() << "; ";
			return ss.str();
		}
	private:
		const SomeSinglton& traits;
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
    (const SomeSinglton&, traits)
	(unsigned int, number)
	(std::string, name)
	(Position, position)
);

template<>
struct SaverProxy<Node> {
	
	template<typename T>
	auto operator()(T& t) -> decltype(t) const
	{
		return t;
	}
	template<typename T>
	T operator()(const T& t) const
	{
		return t;
	}

	SomeSingltonID operator()(const SomeSinglton& t) const
	{
		return getNameOfSomeSinglton(&t);
	}
};




template < class T, class R >
struct ToStdTuple;

template < class... TTypes, class X >
struct ToStdTuple< std::tuple< TTypes... >, X >
{
  typedef std::tuple< TTypes..., X > type;
};


template<typename T>
struct MplSeqToStdTuple {
	   //typedef typename mpl::copy<
		   //T,
		   //mpl::back_inserter<mpl::vector<>>
		   //>::type result;
	typedef typename mpl::fold<T, std::tuple<>, ToStdTuple< mpl::_1, mpl::_2 > >::type type;
};

template<typename T>
struct ConvertSinglton {
		typedef typename mpl::copy<
		   T,
		   mpl::back_inserter<mpl::vector<>>
		   >::type result;
		typedef typename mpl::replace<
			result,
			const SomeSinglton&,
			SomeSingltonID
		>::type type;
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
	const SomeSinglton& operator()(SomeSingltonID& t) {
		return *getSomeSingltonByName(t);
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
	
	typedef typename ConvertSinglton<T>::type MplTType;
	typedef typename MplSeqToStdTuple<MplTType>::type TType;

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
       typedef Constructor<const SomeSinglton& , unsigned int, std::string, Position> wanted_;
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
	NodePtr node1 = boost::make_shared<Node>(*SomeSinglton::instance(),10,"Help",std::make_pair(20, 30));
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

