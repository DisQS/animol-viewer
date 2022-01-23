#pragma once

#include <iostream>


//this file:
//
//	-defines plate::bfloat, a 16-bit floating-point type
//	-creates a specialization of std::numerical_limits for plate::bfloat
//	-defines a nextafter function for plate::bfloats
//
//for all suitable operations/functions, except those explicitly defined here, a plate::bfloat is first losslessly casted
//	(promoted) to a float
//
//a plate::bfloat can be created from a float by truncation of the mantissa (rounding towards 0) in all cases except
//	those where the float is a NaN, in which case a numerical_limits<float>::quiet_NaN is truncated instead (resulting in a
//	quiet NaN plate::bfloat)
//
//unless altered diectly (ie not through functions/operators defined here) a bfloat can only store one NaN, which is a qNaN


namespace plate {

struct bfloat
{
	/*union //TODO: remove if never used
	{
		struct {
			std::uint16_t s_mantissa : 7;
			std::uint16_t s_exponent : 8;
			std::uint16_t s_sign : 1;
		};

		struct {
			std::uint16_t s0 : 1;
			std::uint16_t s1 : 1;
			std::uint16_t s2 : 1;	
			std::uint16_t s3 : 1;
			std::uint16_t s4 : 1;
			std::uint16_t s5 : 1;
			std::uint16_t s6 : 1;
			std::uint16_t s7 : 1;
			std::uint16_t s8 : 1;
			std::uint16_t s9 : 1;
			std::uint16_t s10 : 1;
			std::uint16_t s11 : 1;
			std::uint16_t s12 : 1;
			std::uint16_t s13 : 1;
			std::uint16_t s14 : 1;
			std::uint16_t s15 : 1;
		};

		std::uint16_t s;
	};*/

	//stores the value
	std::uint16_t s;


	//default constructor
	consteval bfloat() = default;

	//copy constructor
	constexpr bfloat(const bfloat& b) = default;

	//conversion constructor
	template<class T>
	constexpr bfloat(const T& t) noexcept
	{
		operator=(t);
	}


	//convert to float (typically used before any arithmetic is performed)
	operator float() const noexcept
	{
		//create float to return
		float f;

		//set first two bytes (least significant 16 bits of mantissa) to 0s
		std::memset(&f, 0, 2);

		//set last two bytes (most significant 7 bits of mantissa, 8-bit exponent, and 1-bit sign) to those of the bfloat
		std::memcpy(reinterpret_cast<std::byte*>(&f)+2, &s, sizeof(s));

		return f;
	};


	//copy assignment operator
	bfloat& operator= (const bfloat& b) = default;

	//conversion assignment operator
	template<class T>
	bfloat& operator= (const T& t) noexcept
	{
		float f = t;

		//check for nan, in which case set this to a nan
		if (std::isnan(f)) [[unlikely]]
		{
			//set f to a qNaN which can be stored in bfloat
			f = std::numeric_limits<float>::quiet_NaN();

			//extract last two bytes (most significant 7 bits of mantissa, 8-bit exponent, and 1-bit sign)
			std::memcpy(&s, reinterpret_cast<std::byte*>(&f)+2, sizeof(s));

			return *this;
		}

		//extract last two bytes (most significant 7 bits of mantissa, 8-bit exponent, and 1-bit sign)
		std::memcpy(&s, reinterpret_cast<std::byte*>(&f)+2, sizeof(s));

		//check for inf, in which case we are done (no rounding needed)
		if (std::isinf(f)) [[unlikely]] return *this;

		//increment the absolute value of s if most significant bit not used is '1' (ie round away from zero)
		if (*(reinterpret_cast<std::uint8_t*>(&f)+1) & std::uint8_t(1)) s += short{1};

		return *this;
	}

};


//these are required due to byte-level operations to interconvert between float and bfloat (ensures the memcyps are safe)
static_assert(sizeof(bfloat) == 2);
static_assert(sizeof(float)  == 4);


} //namespace plate


namespace std {

//nextafter on plate::bfloats (returns the next representable value after b in the direction of direction)
inline plate::bfloat nextafter(plate::bfloat b, plate::bfloat direction)
{
	if (direction > b)
	{
		if (b >= 0)	      b.s += short{1};
		else              b.s -= short{1};
	}
	else if (direction < b)
	{
		if      (b == 0) {b.s += short{1}; b = -b;} //special case - increments b.s to denorm_min, then negates
		else if (b >  0)	b.s -= short{1};
		else              b.s += short{1};
	}
	else //special cases - b and direction are the same, or one (of b and direction) is a NaN
	{
		if (!std::isnan(b)) return direction;
	}

	return b;
}

} //namespace std



//create a specialization of std::numerical_limits for plate::bfloat
template<>
class std::numeric_limits<plate::bfloat> {

public:

	static constexpr bool is_specialized = true;

	static constexpr bool is_signed  = true;
	static constexpr bool is_integer = false;
	static constexpr bool is_exact   = false;

	static constexpr bool               has_infinity      = true;
	static constexpr bool               has_quiet_NaN     = true;
	static constexpr bool               has_signaling_NaN = false; //cannot STORE an sNaN
	static constexpr float_denorm_style has_denorm        = std::numeric_limits<float>::has_denorm;
	static constexpr bool               has_denorm_loss   = false; //TODO: research

	static constexpr std::float_round_style round_style = std::round_indeterminate;
	//a std::round_to_nearest (float) round is performed after every calculation, and
	//a std::round_to_nearest (bfloat) round is performed upon storage

	static constexpr bool is_iec559  = false;
	static constexpr bool is_bounded = true;
	static constexpr bool is_modulo  = false;

	static constexpr int digits       = 8;
	static constexpr int digits10     = 2;
	static constexpr int max_digits10 = 4;

	static constexpr int radix = 2;

	static constexpr int min_exponent   = std::numeric_limits<float>::min_exponent;
	static constexpr int min_exponent10 = std::numeric_limits<float>::min_exponent10;
	static constexpr int max_exponent   = std::numeric_limits<float>::max_exponent;
	static constexpr int max_exponent10 = std::numeric_limits<float>::max_exponent10;

	static constexpr bool traps = false;

	static constexpr bool tinyness_before = std::numeric_limits<float>::tinyness_before;


	//these are are as compile-time as possible, and are all the same as for floats except:
	//	-epsilon(), which is calculated explicitly as the difference between 1 and the next storable value greater than 1
	//	-round_error(), which is 1 due to the rounding towards zero performed on conversion from float to plate::bfloat
	//	-denorm_min(), which is 2^16 times higher than float's, as plate::bfloat has 16 less bits of mantissa
	//	-max(), and lowest(), which are one less and more respectively than infinity and -infinity respectively
	static consteval plate::bfloat min()         noexcept { return std::numeric_limits<float>::min(); }

	static           plate::bfloat lowest()      noexcept {
																				return std::nextafter(
		                                      static_cast<plate::bfloat>(-std::numeric_limits<plate::bfloat>::infinity()),
		                                      static_cast<plate::bfloat>(0) );
	}

	static           plate::bfloat max()         noexcept {
		                                    return std::nextafter(
		                                      std::numeric_limits<plate::bfloat>::infinity(),
		                                      static_cast<plate::bfloat>(0));
	}

	static           plate::bfloat epsilon()     noexcept { plate::bfloat next = 1; next.s++; return next - 1.f; }

	static consteval plate::bfloat round_error() noexcept { return 1; }

	static constexpr plate::bfloat infinity()	   noexcept	{ return std::numeric_limits<float>::infinity(); }

	static consteval plate::bfloat quiet_NaN()   noexcept	{ return std::numeric_limits<float>::quiet_NaN(); }

	static constexpr plate::bfloat denorm_min()  noexcept	{ return (1<<16) * std::numeric_limits<float>::denorm_min(); }

	//note the lack of signaling_NaN(), as this plate::bfloat does not store signaling NaNs (so has_signaling_NaN is false)
};






















//TODO: REMOVE EVERYTHING FROM HERE ONWARDS (OR MOVE TO HELPER FILE)!!!!!!!!!!!****************





/*void print_float(float f) {
	std::cout << (unsigned int)*(std::uint8_t *)&f << " " << (unsigned int)*(((std::uint8_t *)&f) + 1) << " " <<  (unsigned int)*(((std::uint8_t *)&f) + 2) << " " << (unsigned int)*(((std::uint8_t *)&f) + 3) << "\n";
}*/

//void print_bfloat(plate::bfloat b) {
//	for (std::uint32_t i = 0; i < 16; i++){
//		if (i == 7 || i == 15) std::cout << " ";
//		std::cout << (false!=((1<<i) & b.s));
//		
//	}
//	std::cout << "  bfloat: " << b << std::endl;// << ":" << std::endl << "        bits: ";/* << b << std::endl;*/
//	/*for (std::uint32_t i = 1; i < 128; i = i << 1){
//		std::cout << (false!=(i & b.s_mantissa));
//	}
//	std::cout << std::endl;*/
//}

//TODO: REMOVE!!!!!!!!!!!****************
//void print_float(float f) {
//
//	std::uint8_t* p = reinterpret_cast<std::uint8_t*>(&f);
//
//	int c = 0;
//	for (int i = 0; i < 4; i++)
//	{
//		for (int j = 0; j < 8; j++)
//		{
//			printf("%s", (p[i] & (1<<j)) ? "1" : "0");
//			c++;
//
//			if (c == 23 || c == 31) printf(" ");
//		}
//	}
//
//
//
//	//std::bitset<32> b =(std::bitset<32>)*(std::uint32_t *)&f;
//	/*std::cout << "float: " << f << ":" << std::endl << "        bits: " << b[31] << " ";
//	for (int i = 30; i > 7; i--){
//		std::cout << b[i];
//	}
//	std::cout << " ";
//	for (int i = 7; i > -1; i--){
//		std::cout << b[i];
//	}
//	std::cout << std::endl;*/
//
//	std::cout << "   float: " << f << std::endl;// << ":" << std::endl << "        bits: ";/* << b << std::endl;*/

///*
//	std::cout << " ";
//	for (int i = 31; i >= 0; i--){
//		std::cout << b[i];
//		if (i == 31 || i == 23) {std::cout << " ";}
//	}
//	std::cout << std::endl;*/
//	std::cout << "                ";
//	print_bfloat(f);
//	std::cout << std::endl;
//
//	//std::cout << (std::bitset<8>)*(std::uint8_t *)&f << " " << (unsigned int)*(((std::uint8_t *)&f) + 1) << " " <<  (unsigned int)*(((std::uint8_t *)&f) + 2) << " " << (unsigned int)*(((std::uint8_t *)&f) + 3) << "\n";
//
//}



