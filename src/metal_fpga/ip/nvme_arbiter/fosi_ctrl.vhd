library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.fosi_util.all;

package fosi_ctrl is

  subtype t_Logic is std_logic;
  subtype t_Logic_v is unsigned;

  subtype t_Handshake_ms is std_logic;
  subtype t_Handshake_sm is std_logic;
  subtype t_Handshake_v_ms is unsigned;
  subtype t_Handshake_v_sm is unsigned;

  -------------------------------------------------------------------------------
  -- Axi Interface: Ctrl
  -------------------------------------------------------------------------------
  --Scalars:

  constant c_CtrlDataWidth : integer := 32;
  subtype t_CtrlData is unsigned (c_CtrlDataWidth-1 downto 0);

  constant c_CtrlStrbWidth : integer := c_CtrlDataWidth/8;
  subtype t_CtrlStrb is unsigned (c_CtrlStrbWidth-1 downto 0);

  constant c_CtrlAddrWidth : integer := 32;
  subtype t_CtrlAddr is unsigned (c_CtrlAddrWidth-1 downto 0);

  constant c_CtrlRespWidth : integer := 2;
  subtype t_CtrlResp is unsigned (c_CtrlRespWidth-1 downto 0);


  --Complete Bundle:
  type t_Ctrl_ms is record
    awaddr   : t_CtrlAddr;
    awvalid  : std_logic;
    wdata    : t_CtrlData;
    wstrb    : t_CtrlStrb;
    wvalid   : std_logic;
    bready   : std_logic;
    araddr   : t_CtrlAddr;
    arvalid  : std_logic;
    rready   : std_logic;
  end record;
  type t_Ctrl_sm is record
    awready  : std_logic;
    wready   : std_logic;
    bresp    : t_CtrlResp;
    bvalid   : std_logic;
    arready  : std_logic;
    rdata    : t_CtrlData;
    rresp    : t_CtrlResp;
    rlast    : std_logic;
    rvalid   : std_logic;
  end record;
  constant c_CtrlNull_ms : t_Ctrl_ms := (
    awaddr   => (others => '0'),
    awvalid  => '0',
    wdata    => (others => '0'),
    wstrb    => (others => '0'),
    wvalid   => '0',
    bready   => '0',
    araddr   => (others => '0'),
    arvalid  => '0',
    rready   => '0' );
  constant c_CtrlNull_sm : t_Ctrl_sm := (
    awready  => '0',
    wready   => '0',
    bresp    => (others => '0'),
    bvalid   => '0',
    arready  => '0',
    rdata    => (others => '0'),
    rresp    => (others => '0'),
    rlast    => '0',
    rvalid   => '0' );

end fosi_ctrl;
