window.storefrontData = {
  brandName: "Store Manage",
  accounts: [
    {
      accountId: "account-001",
      username: "client001",
      passwordHash: "hash:123456",
      role: "Customer",
      status: "Active",
      customerId: "customer-001"
    },
    {
      accountId: "account-002",
      username: "staff001",
      passwordHash: "hash:staff123",
      role: "Staff",
      status: "Active",
      employeeId: "employee-001"
    },
    {
      accountId: "account-003",
      username: "admin001",
      passwordHash: "hash:admin123",
      role: "Admin",
      status: "Active",
      employeeId: "employee-002"
    }
  ],
  customers: [
    {
      customerId: "customer-001",
      accountId: "account-001",
      fullName: "Nguyen Van A",
      phone: "0900000000",
      city: "Ho Chi Minh City",
      address: {
        recipientName: "Nguyen Van A",
        phone: "0900000000",
        line1: "12 Nguyen Hue",
        line2: "",
        ward: "Ben Nghe",
        district: "District 1",
        city: "Ho Chi Minh City",
        country: "Vietnam"
      },
      wishlist: []
    }
  ],
  employees: [
    {
      employeeId: "employee-001",
      accountId: "account-002",
      fullName: "Store Staff",
      position: "Sales Staff"
    },
    {
      employeeId: "employee-002",
      accountId: "account-003",
      fullName: "System Admin",
      position: "Administrator"
    }
  ],
  products: [
    {
      productId: "product-001",
      name: "Structured Coat",
      category: "Outerwear",
      description: "Long-line black coat with quiet shoulder structure, fluid drape, and a disciplined silhouette for day-to-evening wear.",
      priceMinor: 4900000,
      images: [
        "https://images.unsplash.com/photo-1529139574466-a303027c1d8b?auto=format&fit=crop&w=1200&q=80",
        "https://images.unsplash.com/photo-1483985988355-763728e1935b?auto=format&fit=crop&w=1200&q=80"
      ],
      variants: [
        { variantId: "variant-001-black-s", size: "S", color: "Black", sku: "COAT-BLK-S", stockQuantity: 10 },
        { variantId: "variant-001-black-m", size: "M", color: "Black", sku: "COAT-BLK-M", stockQuantity: 12 },
        { variantId: "variant-001-black-l", size: "L", color: "Black", sku: "COAT-BLK-L", stockQuantity: 8 },
        { variantId: "variant-001-ivory-s", size: "S", color: "Ivory", sku: "COAT-IVR-S", stockQuantity: 9 },
        { variantId: "variant-001-ivory-m", size: "M", color: "Ivory", sku: "COAT-IVR-M", stockQuantity: 9 }
      ]
    }
  ],
  voucher: {
    code: "WELCOME10",
    rate: 0.10,
    maxDiscountMinor: 800000,
    minOrderMinor: 1000000
  }
};
