#ifndef REGULATOR_HPP
#define REGULATOR_HPP

template <class value_type>
struct regulator_t
{
	typedef uint8_t coef_type;
	typedef int32_t int_type;
	
	regulator_t(const coef_type& p = 1, const coef_type& i = 0, const coef_type& d = 0, const value_type& max_output = 0)
		:m_w(0), m_lw(0), m_stop(false), m_p(p), m_i(i), m_d(d),
		 m_e(0), m_le(0), m_de(0), m_x(0), m_y(0), m_integrator(0), m_max_output(max_output)
	{}
	
	regulator_t<value_type>& operator ()(value_type v)
	{
		m_lw = m_w;
		m_w = v;
		return *this;
	}
	
	void P(const coef_type& v) { m_p = v; }
	coef_type P() const { return m_p; }
	void I(const coef_type& v) { m_i = v; }
	coef_type I() const { return m_i; }
	void D(const coef_type& v) { m_d = v; }
	coef_type D() const { return m_d; }
	
	void stop(bool s = true)
	{
		m_stop = s;
		m_x = 0;
		m_w = 0;
		m_lw = 0;
		m_e = 0;
		m_le = 0;
		m_de = 0;
		m_integrator = 0;
	}
	bool is_stopped() const { return m_stop; }
	
	value_type process(const value_type& y)
	{
		m_y = y;
		
		if(m_stop)
			return 0;
		m_e = m_w - y;	//m_w - required  value, y - encoder position/distance
		
		int_type integrator = m_integrator;
		
		const bool stable = m_w == m_lw;
		
		if((int_type(m_e) && 0x80000000) != (int_type(m_le) && 0x80000000))
		{
			integrator = ((int_type(m_x) << 4) - m_p * m_le - m_d * m_de) / m_i;
			m_integrator = integrator;
		}
		else if (m_i != 0 && (!stable || avrlib::abs(m_e) > 10))
			 integrator += m_e;
		
		m_de = m_e - m_le;
	
		int_type x  = ((int_type(m_p) * m_e )       >> 4)
					+ ((int_type(m_d) * m_de)       >> 4)
					+ ((int_type(m_i) * integrator) >> 4);
			
		if(x > m_max_output)
		{
			m_x = m_max_output;
			if(integrator < 0)
				m_integrator = integrator;
		}
		else if(x < -m_max_output)
		{
			m_x = -m_max_output;
			if(integrator > 0)
				m_integrator = integrator;
		}
		else
		{
			m_x = x;
			m_integrator = integrator;
		}
		m_le = m_e;
		
		if(avrlib::abs(m_x) < ((m_max_output * 3) >> 6))
		{
			m_x = 0;
			m_integrator = 0;
		}
		
		return m_x;
	}
	
	void clear()
	{
		m_integrator = 0;
		m_le = m_e;
		m_w = 0;
	}
	
	value_type operator ()() { return m_w; }
	value_type e()  const { return m_e; }
	value_type x()  const { return m_x; }
	value_type y()  const { return m_y; }
	value_type w()  const { return m_w; }
	value_type de() const { return m_de; }
	int_type integrator() const { return m_integrator; }
	
	void max_output(const value_type& v) { m_max_output = v; }
	value_type max_output() const { return m_max_output; }
	
private:
	volatile value_type m_w;
	volatile value_type m_lw;
	volatile bool m_stop;
	volatile coef_type m_p;
	volatile coef_type m_i;
	volatile coef_type m_d;
	volatile value_type m_e;
	volatile value_type m_le;
	volatile value_type m_de;
	volatile value_type m_x;
	volatile value_type m_y;
	volatile int_type m_integrator;
	volatile value_type m_max_output;	//crop output, absolute value
};


#endif