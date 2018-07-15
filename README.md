# moon_c
문c 블로그 with ARM64 Linux Kernel 4.x
http://jake.dothome.co.kr

* char
  - new_gpio_api.c  gpio 캐릭터 디바이스로 gpio 핀 제어 테스트 샘플  (파일을 직접 열고 ioctl로 제어)
  - new_gpio_api2.c  gpio 캐릭터 디바이스로 gpio 핀 제어 테스트 샘플 (gpiod api 사용)
  
  
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
  

* foo-gpio-pci 디렉토리
  - gpio.c        
    - pci 플랫폼 드라이버에 gpio 컨트롤러를 연동하고 legacy 인터럽트 연동 샘플


* msi 디렉토리
  - gpio.c          
    - pci 플랫폼 드라이버에 gpio 컨트롤러를 연동하고 msi 인터럽트 연동 샘플 (홀딩)


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

