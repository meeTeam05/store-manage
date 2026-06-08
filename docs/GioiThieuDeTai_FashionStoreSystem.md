# FASHION STORE SYSTEM

**Đề tài:** Website quản lý và bán hàng thời trang

Hệ thống hỗ trợ khách hàng xem sản phẩm, tìm kiếm, thêm vào giỏ hàng, đặt hàng và thanh toán. Bên cạnh đó, nhân viên và quản lý có thể quản lý sản phẩm, kho hàng, đơn hàng, khuyến mãi và báo cáo doanh thu.

---

## Cấu trúc module

```
FashionStoreSystem
│
├── AccountModule
│   ├── Account
│   ├── CustomerAccount
│   ├── EmployeeAccount
│   └── AuthManager
│
├── ProductModule
│   ├── Product
│   ├── Clothing
│   ├── Shoes
│   └── Accessory
│
├── SizeModule
│   ├── Size
│   ├── Color
│   └── SizeChart
│
├── InventoryModule
│   ├── InventoryItem
│   └── InventoryManager
│
├── CustomerModule
│   ├── Customer
│   ├── Wishlist
│   └── CustomerManager
│
├── CartModule
│   ├── Cart
│   └── CartItem
│
├── OrderModule
│   ├── Order
│   ├── OrderDetail
│   └── OrderManager
│
├── PaymentModule
│   ├── Payment
│   ├── CashPayment
│   ├── BankTransferPayment
│   └── EWalletPayment
│
├── PromotionModule
│   ├── Promotion
│   ├── Voucher
│   └── PromotionManager
│
├── ReviewModule
│   ├── Review
│   ├── Rating
│   └── ReviewManager
│
├── ShippingModule
│   ├── ShippingAddress
│   ├── ShippingMethod
│   └── ShippingManager
│
├── ReturnModule
│   ├── ReturnRequest
│   └── ReturnManager
│
├── EmployeeModule
│   ├── Employee
│   ├── SalesStaff
│   ├── Manager
│   └── Admin
│
├── ReportModule
│   ├── RevenueReport
│   ├── ProductReport
│   └── ReportManager
│
└── FileModule
    └── FileManager
```

---

## Chức năng chính

- Quản lý tài khoản và phân quyền: khách hàng, nhân viên, quản lý, admin.
- Quản lý sản phẩm thời trang theo danh mục, thương hiệu, size, màu sắc và hình ảnh.
- Quản lý kho hàng theo từng biến thể sản phẩm như size và màu.
- Tìm kiếm, lọc và sắp xếp sản phẩm theo giá, size, màu, danh mục.
- Giỏ hàng cho phép thêm, xóa, cập nhật số lượng và tính tổng tiền.
- Đặt hàng, quản lý chi tiết đơn hàng và trạng thái đơn hàng.
- Thanh toán bằng tiền mặt, chuyển khoản hoặc ví điện tử ở mức mô phỏng.
- Áp dụng mã giảm giá và chương trình khuyến mãi.
- Đánh giá sản phẩm bằng sao và bình luận sau khi mua hàng.
- Gửi yêu cầu đổi trả hàng khi sai size, lỗi sản phẩm hoặc không đúng mô tả.
- Báo cáo doanh thu, sản phẩm bán chạy và hàng tồn kho thấp.

---

## Liên hệ OOP

Hệ thống phù hợp với lập trình hướng đối tượng vì mỗi thành phần được biểu diễn thành class riêng. Có thể áp dụng kế thừa như:

- `Product` → `Clothing` / `Shoes` / `Accessory`
- `Payment` → `CashPayment` / `BankTransferPayment` / `EWalletPayment`
- `Employee` → `SalesStaff` / `Manager` / `Admin`

Các module giúp chương trình dễ mở rộng, dễ bảo trì và thể hiện rõ **đóng gói**, **kế thừa**, **đa hình** và **trừu tượng hóa**.
