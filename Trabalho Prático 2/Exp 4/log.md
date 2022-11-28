# Exp 4

## Questions

### How to configure a static route in a commercial router?
### What are the paths followed by the packets in the experiments carried out and why?
### How to configure NAT in a commercial router?
### What does NAT do?

## Experiment Summary

### Conectar router (Rc) à rede do laboratorio
Configurar Router
* Connect S0 port to RS232 on the patch panel and from RS232 to the console port of the switch.
* Set the Baudrate to 115200 bps
* Login (username: admin, password: blanck)
* /system reset-configuration

Configurar endereço ip através da consola do router
```
/ip address add address=172.16.1.29/24 interface=ether0
/ip address add address=172.16.1.254/24 interface=ether1
/ip address print
```
![Ip Adress Configuration](ipconfig.png)

### Adicionar default routes
tux24 como default router para o tux23
```bash
route add default gw 172.16.20.254
```

Rc como default router para tux22 e tux24
```bash
route add default gw 172.16.21.254
```

No tux22 e no Rc adicionar routes para 172.16.Y0.0/24
```bash
route add default gw 172.16.21.253
```

### Verificar que tux23 consegue fazer ping a todas as network interfaces

ping tux24
![Ping tux24](Screenshot%20at%202022-11-24%2013-00-49.png)
ping tux22 and Rc
![Ping tux22 and Rc2](Screenshot%20at%202022-11-24%2013-00-56.png)

