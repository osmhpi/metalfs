library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
-- i_configStore : entity work.ConfigStore
--   generic map (
--     g_ConstData => x"",
--     g_ConstDataLength => x"0000015f")
--   port map (
--     p_clk => ,
--     ... );
entity ConfigStore is
  generic (
    g_ConstData       : unsigned (4096*8-1 downto 0);
    g_ConstDataLength : unsigned (31 downto 0));
  port (
    p_clk        : in  std_logic;
    p_rst_n      : in  std_logic;
    -- layout
    --  0x0000: length
    --  0x1000..1FFF: data
    p_regs_AWADDR   : in  unsigned (11 downto 0);
    p_regs_AWVALID  : in  std_logic;
    p_regs_AWREADY  : out std_logic;
    p_regs_WDATA    : in  unsigned (31 downto 0);
    p_regs_WSTRB    : in  unsigned ( 3 downto 0);
    p_regs_WVALID   : in  std_logic;
    p_regs_WREADY   : out std_logic;
    p_regs_BRESP    : out unsigned (1 downto 0);
    p_regs_BVALID   : out std_logic;
    p_regs_BREADY   : in  std_logic;
    p_regs_ARADDR   : in  unsigned (11 downto 0);
    p_regs_ARVALID  : in  std_logic;
    p_regs_ARREADY  : out std_logic;
    p_regs_RDATA    : out unsigned (31 downto 0);
    p_regs_RRESP    : out unsigned (1 downto 0);
    p_regs_RVALID   : out std_logic;
    p_regs_RREADY   : in  std_logic);
end ConfigStore;
architecture ConfigStore of ConfigStore is
  type t_State is (Idle, ReadAck);
  signal s_state : t_State;
  subtype t_Addr is integer range 0 to 2**10-1;
  constant c_AddrBase : t_Addr := 2**9;
  signal s_araddr : t_Addr;
begin
  p_regs_AWREADY <= '0';
  p_regs_WREADY  <= '0';
  p_regs_BRESP   <= "00";
  p_regs_BVALID  <= '0';
  s_araddr <= to_integer(p_regs_ARADDR(11 downto 2));
  process (p_clk)
  begin
    if p_clk'event and p_clk = '1' then
      if p_rst_n = '0' then
        s_state <= Idle;
      else
        if s_state = Idle and p_regs_ARVALID = '1' then
          s_state <= ReadAck;
          if s_araddr < c_AddrBase then
            p_regs_RDATA <= g_ConstDataLength;
          else
            p_regs_RDATA <= g_ConstData((s_araddr-c_AddrBase)*32+31 downto (s_araddr-c_AddrBase)*32);
          end if;
        elsif s_state = ReadAck and p_regs_RREADY = '1' then
          s_state <= Idle;
        end if;
      end if;
    end if;
  end process;
  with s_state select p_regs_ARREADY <=
    '0' when ReadAck,
    '1' when others;
  p_regs_RRESP <= "00";
  with s_state select p_regs_RVALID <=
    '1' when ReadAck,
    '0' when others;
end ConfigStore;