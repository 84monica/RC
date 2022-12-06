# Exp 6

## Questions

### How many TCP connections are opened by your ftp application?
### In what connection is transported the FTP control information?
### What are the phases of a TCP connection?
### How does the ARQ TCP mechanism work? What are the relevant TCP fields? What relevant information can be observed in the logs?
### How does the TCP congestion control mechanism work? What are the relevant fields. How did the throughput of the data connection evolve along the time? Is it according the TCP congestion control mechanism?
### Is the throughput of a TCP data connections disturbed by the appearance of a second TCP connection? How?

## Experiment Summary

### Correr download no tux23 e verificar se o ficheiro chegou corretamente
[Captura](exp6.pcapng)

### Observar packets 
– TCP control and data connections, and its phases (establishment, data,
termination)
– Data transferred through the FTP control connection
– TCP ARQ mechanism
– TCP congestion control mechanism in action
– Note: use also Wireshark Statistics tools (menu) to study TCP phases, ARQ
and congestion control mechanism

### Correr download no tux23 e a meio da transferencia correr no tux22
Captura!