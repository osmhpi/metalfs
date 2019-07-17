library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


package fosi_util is

  function f_bitCount(v_bits : unsigned) return integer;

  function f_logic(v_bool : boolean) return std_logic;

  function f_encode(v_vect : unsigned) return integer;

  function f_resize(v_vector : unsigned; v_width : integer; v_offset : integer := 0) return unsigned;
  function f_resizeVU(v_vector : std_logic_vector; v_width : integer; v_offset : integer := 0) return unsigned;
  function f_resizeUV(v_vector : unsigned; v_width : integer; v_offset : integer := 0) return std_logic_vector;
  function f_resizeLeft(v_vector : unsigned; v_width : integer; v_offset : integer := 0) return unsigned;

  function f_clog2(v_value : natural) return positive;

  function f_cdiv(v_value : natural; v_div : positive) return positive;

  function f_or(v_bits : unsigned) return std_logic;
  function f_and(v_bits : unsigned) return std_logic;
  function f_xor(v_bits : unsigned) return std_logic;

  function f_byteMux(v_select : unsigned; v_data0 : unsigned; v_data1 : unsigned) return unsigned;


end fosi_util;

package body fosi_util is

  function f_bitCount(v_bits : unsigned) return integer is
    variable v_result : integer range 0 to v_bits'length ;
    variable v_index : integer range v_bits'range;
  begin
    v_result := 0;
    for v_index in v_bits'range loop
      if v_bits(v_index) = '1' then
        v_result := v_result + 1;
      end if;
    end loop;
    return v_result;
  end f_bitCount;

  function f_logic(v_bool : boolean) return std_logic is
  begin
    if v_bool then
      return '1';
    else
      return '0';
    end if;
  end f_logic;

  function f_encode(v_vect : unsigned) return integer is
    variable v_index : integer range v_vect'range;
    variable v_result : integer range v_vect'range;
    variable v_guard : boolean;
  begin
    v_guard := false;
    v_result := v_vect'low;
    for v_index in v_vect'low to v_vect'high loop
      if v_vect(v_index) = '1' and not v_guard then
        v_guard := true;
        v_result := v_index;
      end if;
    end loop;
    return v_result - v_vect'low;
  end f_encode;

  function f_resize(v_vector : unsigned; v_width : integer; v_offset : integer := 0) return unsigned is
    variable v_length : integer := v_vector'length;
    variable v_high : integer := v_vector'high;
    variable v_low : integer := v_vector'low;
    variable v_result : unsigned(v_width-1 downto 0);
  begin
    if v_length <= v_offset then
      v_result := to_unsigned(0, v_width);
    elsif v_length - v_offset < v_width then
      v_result := to_unsigned(0, v_width - (v_length - v_offset)) &
              v_vector(v_high downto v_low + v_offset);
    elsif v_length - v_offset >= v_width then
      v_result := v_vector(v_low + v_offset + v_width - 1 downto v_low + v_offset);
    end if;
    return v_result;
  end f_resize;

  function f_resizeVU(v_vector : std_logic_vector; v_width : integer; v_offset : integer := 0) return unsigned is
  begin
    return f_resize(unsigned(v_vector), v_width, v_offset);
  end f_resizeVU;

  function f_resizeUV(v_vector : unsigned; v_width : integer; v_offset : integer := 0) return std_logic_vector is
  begin
    return std_logic_vector(f_resize(v_vector, v_width, v_offset));
  end f_resizeUV;

  function f_resizeLeft(v_vector : unsigned; v_width : integer; v_offset : integer := 0) return unsigned is
    variable v_length : integer := v_vector'length;
    variable v_high : integer := v_vector'high;
    variable v_low : integer := v_vector'low;
    variable v_result : unsigned(v_width-1 downto 0);
  begin
    if v_length <= v_offset then
      v_result := to_unsigned(0, v_width);
    elsif v_length - v_offset < v_width then
      v_result := v_vector(v_high - v_offset downto v_low) &
                    to_unsigned(0, v_width - (v_length - v_offset));
    elsif v_length - v_offset >= v_width then
      v_result := v_vector(v_high - v_offset downto v_high - v_offset - v_width + 1);
    end if;
    return v_result;
  end f_resizeLeft;

  function f_clog2 (v_value : natural) return positive is
    variable v_count  : positive;
  begin
    v_count := 1;
    while v_value > 2**v_count loop
      v_count := v_count + 1;
    end loop;
    return v_count;
  end f_clog2;

  function f_cdiv(v_value : natural; v_div : positive) return positive is
  begin
    if v_value = 0 then
      return 0;
    else
      return (v_value - 1) / v_div + 1;
    end if;
  end f_cdiv;

  function f_or(v_bits : unsigned) return std_logic is
    variable v_or : std_logic := '0';
  begin
    for i in v_bits'low to v_bits'high loop
      v_or := v_or or v_bits(i);
    end loop;
    return v_or;
  end f_or;

  function f_and(v_bits : unsigned) return std_logic is
    variable v_and : std_logic := '1';
  begin
    for i in v_bits'low to v_bits'high loop
      v_and := v_and and v_bits(i);
    end loop;
    return v_and;
  end f_and;

  function f_xor(v_bits : unsigned) return std_logic is
    variable v_xor : std_logic := '0';
  begin
    for i in v_bits'low to v_bits'high loop
      v_xor := v_xor xor v_bits(i);
    end loop;
    return v_xor;
  end f_xor;

  function f_byteMux(v_select : unsigned; v_data0 : unsigned; v_data1 : unsigned) return unsigned is
    variable v_result : unsigned (v_data0'length-1 downto 0);
    variable v_index : integer range v_select'range;
  begin
    assert v_select'length * 8 = v_data0'length report "f_byteMux arg width mismatch" severity failure;
    assert v_select'length * 8 = v_data1'length report "f_byteMux arg width mismatch" severity failure;

    for v_index in v_select'low to v_select'high loop
      if v_select(v_index) = '1' then
        v_result(v_result'low+v_index*8+7 downto v_result'low+v_index*8) :=
          v_data1(v_data1'low+v_index*8+7 downto v_data1'low+v_index*8);
      else
        v_result(v_result'low+v_index*8+7 downto v_result'low+v_index*8) :=
          v_data0(v_data0'low+v_index*8+7 downto v_data0'low+v_index*8);
      end if;
    end loop;
    return v_result;
  end f_byteMux;

end fosi_util;
