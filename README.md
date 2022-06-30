# moon_c
문c 블로그 with ARM64 Linux Kernel 5.x
http://jake.dothome.co.kr

* char
  - new_gpio_api.c  gpio 캐릭터 디바이스로 gpio 핀 제어 테스트 샘플  (파일을 직접 열고 ioctl로 제어)
  - new_gpio_api2.c  gpio 캐릭터 디바이스로 gpio 핀 제어 테스트 샘플 (gpiod api 사용)
  
* clk 디렉토리
  - clk-foo-divider.c
    - compatible = "foo,divider-clock" 클럭 소스로 커널 컴파일 시 포함(inbuild) 되어야 함.
  - clk-foo-mux.c
    - compatible = "foo,mux-clock" 클럭 소스로 커널 컴파일 시 포함(inbuild) 되어야 함.
  - clk.c
    - 위 클럭 드라이버를 사용한 사용자 샘플
  
* device-1 디렉토리
  - foo.c         
    - foo_bus와 foo_class 등록
  - foo_pdev.c    
    - foo 버스 컨트롤러 플랫폼 디바이스 등록
  - foo_pdrv.c    
    - foo 버스 컨트롤러 플랫폼 드라이버 등록
  - foo_client1.c 
    - foo 클라이언트 디바이스 및 드라이버 등록
  - foo_client2.c 
    - foo_client1.c와 동일하지만 foo_device_register() 및 foo_driver_register() 사용


* device-2 디렉토리
  - foo.c         
    - foo 디바이스 등록과 with 디바이스 속성
  - foo2.c
    - foo2 디바이스 등록 with 디바이스 속성(플랫폼 디바이스로 등록)
  - foo3.c
    - foo3 플랫폼 디바이스 등록
    - 디바이스 속성 추가
    - 플랫폼 리소스 등록(iomem, irq)
  - drv3.c 
    - drv3 플랫폼 드라이버 등록(foo3 디바이스와 pair)
    - 드라이버 속성 추가
    - 플랫폼 리소스를 얻어와서 사용하는 방법  
  - drv4.c 
    - drv4 플랫폼 드라이버 등록(virt.dts의 foo4 노드에서 등록시킨 플랫폼 디바이스와 pair)
    - 드라이버 속성 추가
    - 디바이스 트리와 연동하여 플랫폼 리소스를 얻어와서 사용하는 방법
    - 디바이스 트리 노드에서 custom 속성 알아오는 방법
  - virt.dts
    - qemu용 default virt 디바이스 트리에 foo4 노드를 추가 (drv4.c와 연동)

* foo-pinctrl 디렉토리
  - foo-pinctrl.c 
    - pinctrl 플랫폼 드라이버 등록(virt.dts의 foo-pinctrl 플랫폼 디바이스와 pair)
    - pinctrl 플랫폼 드라이버에 8개의 pin, 4개의 function 및 다수의 그룹 등록
    - 디바이스 트리를 사용하여 pinmux/pinconf 매핑 사용
    - pictrl 드라이버가 로딩되면서 디폴트 pinmux/pinconf 매핑 동작
  - foo-pinctrl2.c 
    - foo-pinctrl.c를 구현만 약간 변형한 버전 (동일한 기능)      
  - virt.dts
    - qemu용 default virt 디바이스 트리에 foo-pinctrl 노드를 추가 (foo-pinctrl.c와 연동)


* foo-gpio 디렉토리
  - gpio.c        
    - gpio 컨트롤러에 legacy 인터럽트 연동 샘플
  - gpio2.c        
    - gpio 컨트롤러에 legacy 인터럽트 연동 샘플-2 
    - 디바이스트리에서 다수의 gpio controller 드라이버 지정 가능
    - 가상 gpio 레지스터 시뮬레이션    
- virt.dts
    - gpio2용 gpio0 & gpio1 노드 추가
  
* foo-gpio-pci 디렉토리
  - gpio.c        
    - pci 플랫폼 드라이버에 gpio 컨트롤러를 연동하고 legacy 인터럽트 연동 샘플

* sysfs 디렉토리
  - foo1.c        
    - sysfs 속성 및 이벤트 발생 테스트 (kobject_add 사용)
  - foo2.c        
    - sysfs 속성 및 이벤트 발생 테스트 (kobject_create_and_add & sysfs_create_group 사용)
  - foo3.c        
    - sysfs 속성 및 이벤트 발생 테스트 (foo 플랫폼 디바이스로 등록)
  - test.c        
    - foo1.c 및 foo2.c의 이벤트 수신 application
  - test3.c       
    - foo3.c 이벤트 수신 application

* hrtimer 디렉토리
  - foo.c
    - pinned hrtimer를 테스트하기 위해 각각의 cpu에서 1초 주기로 hrtimer를 동작
  
* foo-i2c 디렉토리
  - foo-i2c.c
    - i2c host controller 에뮬레이션 샘플

* foo-i2c-1 디렉토리
  - foo-lcd.c
    - i2c client 에뮬레이션 샘플
    - 0x3f 주소를 갖는 1602-LCD-I2C(16x2)의 백라이트를 제어

* foo-pci-1 디렉토리
  - foo-pci-1.c
    - pci 디바이스 덤프

* foo-pci-2 디렉토리
  - foo-pci-2.c
    - ivshmem pci 디바이스를 사용한 가상 gpio 컨트롤러 구현
    - chained irq & nested irq test for legacy IRQx 
    - MSI-x enable  

* tcp-1 디렉토리
  - tcp server example (select 사용, queue 및 예외 처리 로직 등을 추가하여 실전 서버 소켓과 유사하게 구현)
  - telnet과 연동 테스트용
  - 실전 서버에는 이 샘플 코드 외에 구조체 전송, magic number 및 seq_id 비교 로직등이 추가 구현됨.
 
* foo-proc 디렉토리
  - proc 구현 샘플

* kernel_threadd 디렉토리
  - FIFO 또는 RR rt 커널 스레드 샘플

* hello_module 디렉토리
  - hello 모듈 샘플

* test/provider 디렉토리
  - barrier test for user application

* file-1 디렉토리
  - 커널에서 유저용 파일 기록(비권장)

* netlink 디렉토리
  - mcast 디렉토리
  - ucast

