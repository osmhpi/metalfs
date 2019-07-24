library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity SnappyWrapper is
  port (
    ap_clk                : in  std_logic;
    ap_rst_n              : in  std_logic;

    interrupt             : out std_logic;

    axis_input_TDATA      : in  std_logic_vector (511 downto 0);
    axis_input_TKEEP      : in  std_logic_vector  (63 downto 0);
    axis_input_TLAST      : in  std_logic;
    axis_input_TVALID     : in  std_logic;
    axis_input_TREADY     : out std_logic;

    axis_output_TDATA     : out std_logic_vector (511 downto 0);
    axis_output_TKEEP     : out std_logic_vector  (63 downto 0);
    axis_output_TLAST     : out std_logic;
    axis_output_TVALID    : out std_logic;
    axis_output_TREADY    : in  std_logic;

    -- Address Map:
    --  0x00 : Control Register
    --  0x00[0] Start Bit
    --  0x00[1] Done Bit
    --  0x00[2] Idle Bit
    --  0x00[3] Ready Bit
    --  0x10 : Input Length (Lower Half)
    --  0x14 : Input Length (Upper Half)
    --  0x18 : Output Length (Lower Half)
    --  0x1C : Output Length (Upper Half)
    s_axi_control_AWADDR  : in  std_logic_vector   (7 downto 0);
    s_axi_control_AWVALID : in  std_logic;
    s_axi_control_WDATA   : in  std_logic_vector  (31 downto 0);
    s_axi_control_WSTRB   : in  std_logic_vector   (3 downto 0);
    s_axi_control_WVALID  : in  std_logic;
    s_axi_control_BREADY  : in  std_logic;
    s_axi_control_ARADDR  : in  std_logic_vector   (7 downto 0);
    s_axi_control_ARVALID : in  std_logic;
    s_axi_control_RREADY  : in  std_logic;
    s_axi_control_AWREADY : out std_logic;
    s_axi_control_WREADY  : out std_logic;
    s_axi_control_BRESP   : out std_logic_vector   (1 downto 0);
    s_axi_control_BVALID  : out std_logic;
    s_axi_control_ARREADY : out std_logic;
    s_axi_control_RDATA   : out std_logic_vector  (31 downto 0);
    s_axi_control_RRESP   : out std_logic_vector   (1 downto 0);
    s_axi_control_RVALID  : out std_logic);
end SnappyWrapper;

architecture SnappyWrapper of SnappyWrapper is

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

  function f_byteswap(v_input: std_logic_vector) return std_logic_vector is
    constant c_input_bits : integer := v_input'length;
    constant c_input_bytes : integer := c_input_bits / 8;
    variable v_return : std_logic_vector(c_input_bits-1 downto 0);
  begin
    for v_idx in 0 to c_input_bytes-1 loop
      v_return(8*v_idx + 7 downto 8*v_idx) := v_input(c_input_bits-8*v_idx-1 downto c_input_bits-8*v_idx-8);
    end loop;
    return v_return;
  end f_byteswap;

  function f_bitswap(v_input: std_logic_vector) return std_logic_vector is
    constant c_input_bits : integer := v_input'length;
    variable v_return : std_logic_vector(c_input_bits-1 downto 0);
  begin
    for v_idx in 0 to c_input_bits-1 loop
      v_return(v_idx) := v_input(c_input_bits-v_idx-1);
    end loop;
    return v_return;
  end f_bitswap;

  -- Decompressor Module:
  component decompressor is
    port (
      clk                  : in  std_logic;
      rst_n                : in  std_logic;
      -- must remain stable between start and done
      compression_length   : in  std_logic_vector (34 downto 0);
      decompression_length : in  std_logic_vector (31 downto 0);
      start                : in  std_logic;
      done                 : out std_logic;
      last                 : out std_logic;
      data                 : in  std_logic_vector (511 downto 0);
      valid_in             : in  std_logic;
      data_fifo_almostfull : out std_logic;
      data_out             : out std_logic_vector (511 downto 0);
      byte_valid_out       : out std_logic_vector (63 downto 0);
      valid_out            : out std_logic;
      wr_ready             : in  std_logic);
  end component;

  signal s_dcmpInLength  : std_logic_vector (34 downto 0);
  signal s_dcmpOutLength : std_logic_vector (31 downto 0);
  signal s_dcmpStart     : std_logic;
  signal s_dcmpDone      : std_logic;
  signal s_dcmpLast      : std_logic;
  signal s_dcmpInData    : std_logic_vector (511 downto 0);
  signal s_dcmpInValid   : std_logic;
  signal s_dcmpInReady   : std_logic;
  signal s_dcmpInReady_n : std_logic;
  signal s_dcmpOutData   : std_logic_vector (511 downto 0);
  signal s_dcmpOutKeep   : std_logic_vector (63 downto 0);
  signal s_dcmpOutValid  : std_logic;
  signal s_dcmpOutReady  : std_logic;

  -- Wrapper Logic:
  subtype t_ByteCounter is unsigned (31 downto 0);
  constant c_ByteCounterZero : t_ByteCounter := to_unsigned(0, t_ByteCounter'length);
  signal s_tkeepBytes : t_ByteCounter;
  signal s_byteCounter : t_ByteCounter;

  type t_State is (Idle, Starting, Running, DoneInt, Done);
  signal s_state : t_State;

  -- Control Register Slave Logic:
  type   t_RegState is (Idle, WrAck, WrRsp, RdAck, RdRsp);
  signal s_regState : t_RegState;
  signal s_regEventStartSet : std_logic;
  signal s_regEventDoneRead : std_logic;

  signal s_regStart : std_logic;
  signal s_regDone : std_logic;
  signal s_regIdle : std_logic;
  signal s_regReady : std_logic;
  signal s_regInLength      : std_logic_vector (63 downto 0);
  alias  a_regInLengthLo is s_regInLength(31 downto 0);
  alias  a_regInLengthHi is s_regInLength(63 downto 32);
  signal s_regOutLength     : std_logic_vector (63 downto 0);
  alias  a_regOutLengthLo is s_regOutLength(31 downto 0);
  alias  a_regOutLengthHi is s_regOutLength(63 downto 32);

begin

  -----------------------------------------------------------------------------
  -- Decompressor Module
  -----------------------------------------------------------------------------
  i_Decompressor : decompressor
    port map (
      clk => ap_clk,
      rst_n => ap_rst_n,
      compression_length => s_dcmpInLength,
      decompression_length => s_dcmpOutLength,
      start => s_dcmpStart,
      done => s_dcmpDone,
      last => s_dcmpLast,
      data => s_dcmpInData,
      valid_in => s_dcmpInValid,
      data_fifo_almostfull => s_dcmpInReady_n,
      data_out => s_dcmpOutData,
      byte_valid_out => s_dcmpOutKeep,
      valid_out => s_dcmpOutValid,
      wr_ready => s_dcmpOutReady);

  s_dcmpInReady <= not s_dcmpInReady_n;

  -----------------------------------------------------------------------------
  -- Wrapper Logic
  -----------------------------------------------------------------------------
  -- s_dcmpLast ignored

  ---- Input Stream Mapping:
  s_dcmpInData <= f_byteswap(axis_input_TDATA);
  -- axis_input_TKEEP and  axis_input_TLAST ignored
  s_dcmpInValid <= axis_input_TVALID;
  axis_input_TREADY <= s_dcmpInReady;

  ---- Output Stream Mapping:
  s_tkeepBytes <= to_unsigned(f_bitCount(unsigned(s_dcmpOutKeep)), t_ByteCounter'length);

  process (ap_clk)
  begin
    if ap_clk'event and ap_clk = '1' then
      if ap_rst_n = '0' then
        s_byteCounter <= c_ByteCounterZero;
      elsif s_dcmpStart = '1' then
        s_byteCounter <= unsigned(s_dcmpOutLength);
      elsif s_dcmpOutValid = '1' and s_dcmpOutReady = '1' and s_byteCounter > c_ByteCounterZero then
        if s_byteCounter <= s_tkeepBytes then
          s_byteCounter <= c_ByteCounterZero;
        else
          s_byteCounter <= s_byteCounter - s_tkeepBytes;
        end if;
      end if;
    end if;
  end process;

  axis_output_TDATA <= f_byteswap(s_dcmpOutData);
  axis_output_TKEEP <= f_bitswap(s_dcmpOutKeep);
  axis_output_TLAST <= '0' when (s_byteCounter > s_tkeepBytes) else '1';
  axis_output_TVALID <= s_dcmpOutValid;
  s_dcmpOutReady <= axis_output_TREADY;

  ---- Length Register Mapping:
  s_dcmpInLength <= s_regInLength(s_dcmpInLength'length-1 downto 0);
  s_dcmpOutLength <= s_regOutLength(s_dcmpOutLength'length-1 downto 0);

  ---- Control State Machine:
  process (ap_clk)
  begin
    if ap_clk'event and ap_clk = '1' then
      if ap_rst_n = '0' then
        s_state <= Idle;
      else
        case s_state is
          when Idle =>
            if s_regEventStartSet = '1' then
              s_state <= Starting;
            end if;
          when Starting =>
            s_state <= Running;
          when Running =>
            if s_dcmpDone = '1' then
              s_state <= DoneInt;
            end if;
          when DoneInt =>
            if s_regEventDoneRead = '1' then
              s_state <= Idle;
            else
              s_state <= Done;
            end if;
          when Done =>
            if s_regEventDoneRead = '1' then
              s_state <= Idle;
            end if;
        end case;
      end if;
    end if;
  end process;
  with s_state select s_dcmpStart <=
    '1' when Starting,
    '0' when others;
  with s_state select interrupt <=
    '1' when DoneInt,
    '0' when others;
  with s_state select s_regStart <=
    '1' when Starting,
    '0' when others;
  with s_state select s_regDone <=
    '1' when DoneInt,
    '1' when Done,
    '0' when others;
  with s_state select s_regIdle <=
    '0' when Running,
    '1' when others;
  with s_state select s_regReady <=
    '1' when Idle,
    '1' when others;

  -----------------------------------------------------------------------------
  -- Control Register Slave Logic
  -----------------------------------------------------------------------------
  process (ap_clk)
  begin
    if ap_clk'event and ap_clk = '1' then
      if ap_rst_n = '0' then
        s_regEventStartSet <= '0';
        s_regEventDoneRead <= '0';
        s_regInLength <= (others => '0');
        s_regOutLength <= (others => '0');
        s_axi_control_RDATA <= (others => '0');
      else
        s_regEventStartSet <= '0';
        s_regEventDoneRead <= '0';
        case s_regState is

          when Idle =>
            if s_axi_control_AWVALID = '1' and s_axi_control_WVALID = '1' then
              -- Ignore WSTRB for simplicity
              if    s_axi_control_AWADDR = x"00" then -- Control Register
                s_regEventStartSet <= s_axi_control_WDATA(0);
              elsif s_axi_control_AWADDR = x"18" then -- Input Length Lo
                a_regInLengthLo <= s_axi_control_WDATA;
              elsif s_axi_control_AWADDR = x"1C" then -- Input Length Hi
                a_regInLengthHi <= s_axi_control_WDATA;
              elsif s_axi_control_AWADDR = x"20" then -- Output Length Lo
                a_regOutLengthLo <= s_axi_control_WDATA;
              elsif s_axi_control_AWADDR = x"24" then -- Output Length Hi
                a_regOutLengthHi <= s_axi_control_WDATA;
              end if;
              s_regState <= WrAck;
            elsif s_axi_control_ARVALID = '1' then
              if    s_axi_control_ARADDR = x"00" then -- Control Register
                s_axi_control_RDATA <= x"0000000" & s_regReady & s_regIdle & s_regDone & s_regStart;
                s_regEventDoneRead <= '1';
              elsif s_axi_control_ARADDR = x"18" then -- Input Length Lo
                s_axi_control_RDATA <= a_regInLengthLo;
              elsif s_axi_control_ARADDR = x"1C" then -- Input Length Hi
                s_axi_control_RDATA <= a_regInLengthHi;
              elsif s_axi_control_ARADDR = x"20" then -- Output Length Lo
                s_axi_control_RDATA <= a_regOutLengthLo;
              elsif s_axi_control_ARADDR = x"24" then -- Output Length Hi
                s_axi_control_RDATA <= a_regOutLengthHi;
              end if;
              s_regState <= RdAck;
            end if;

          when WrAck =>
            if s_axi_control_BREADY = '1' then
              s_regState <= Idle;
            else
              s_regState <= WrRsp;
            end if;
          when WrRsp =>
            if s_axi_control_BREADY = '1' then
              s_regState <= Idle;
            end if;

          when RdAck =>
            if s_axi_control_RREADY = '1' then
              s_regState <= Idle;
            else
              s_regState <= RdRsp;
            end if;
          when RdRsp =>
            if s_axi_control_RREADY = '1' then
              s_regState <= Idle;
            end if;

        end case;
      end if;
    end if;
  end process;

  with s_regState select s_axi_control_AWREADY <=
    '1' when WrAck,
    '0' when others;
  with s_regState select s_axi_control_WREADY <=
    '1' when WrAck,
    '0' when others;
  s_axi_control_BRESP <= "00"; -- Response Code OK
  with s_regState select s_axi_control_BVALID <=
    '1' when WrAck,
    '1' when WrRsp,
    '0' when others;

  with s_regState select s_axi_control_ARREADY <=
    '1' when RdAck,
    '0' when others;
  s_axi_control_RRESP <= "00"; -- Response Code OK
  with s_regState select s_axi_control_RVALID <=
    '1' when RdAck,
    '1' when RdRsp,
    '0' when others;

end SnappyWrapper;

