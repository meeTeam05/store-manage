const fs = require('fs');
const path = require('path');

const rootDir = path.resolve(__dirname, '..');
const demoDir = path.join(rootDir, 'data', 'demo');
const targetFile = path.join(rootDir, 'web', 'assets', 'js', 'storefront-data.js');

try {
  console.log("Reading data from:", demoDir);
  
  // Read inputs
  const accountsRaw = JSON.parse(fs.readFileSync(path.join(demoDir, 'accounts.json'), 'utf8'));
  const productsRaw = JSON.parse(fs.readFileSync(path.join(demoDir, 'products.json'), 'utf8'));
  const vouchersRaw = JSON.parse(fs.readFileSync(path.join(demoDir, 'vouchers.json'), 'utf8'));

  // 1. Process Accounts, Customers, Employees
  let staffCount = 0;
  const accounts = accountsRaw.map(acc => {
    const mapped = {
      accountId: acc.id,
      username: acc.username,
      passwordHash: acc.password_hash,
      role: acc.role,
      customerId: acc.customer_profile ? acc.customer_profile.id : null,
      employeeId: null
    };
    if (acc.role === "Staff" || acc.role === "Admin") {
      staffCount++;
      mapped.employeeId = `employee-00${staffCount}`;
    }
    return mapped;
  });

  const customers = accountsRaw
    .filter(acc => acc.customer_profile)
    .map(acc => {
      const cp = acc.customer_profile;
      const addr = cp.address || {};
      return {
        customerId: cp.id,
        accountId: acc.id,
        fullName: cp.name,
        phone: cp.phone,
        city: addr.city || "Ho Chi Minh City",
        address: {
          recipientName: addr.recipient_name || cp.name,
          phone: addr.phone || cp.phone,
          line1: addr.line1 || "",
          line2: addr.line2 || "",
          ward: addr.ward || "",
          district: addr.district || "",
          city: addr.city || "",
          country: addr.country || "Vietnam"
        },
        wishlist: []
      };
    });

  const employees = accounts
    .filter(acc => acc.role === "Staff" || acc.role === "Admin")
    .map(acc => {
      const isStaff = acc.role === "Staff";
      return {
        employeeId: acc.employeeId,
        accountId: acc.accountId,
        fullName: isStaff ? "Store Staff" : "System Admin",
        position: isStaff ? "Sales Staff" : "Administrator"
      };
    });

  // 2. Process Products
  const products = productsRaw.map(p => {
    const prices = p.variants.map(v => v.price).filter(price => typeof price === 'number' && price > 0);
    const priceMinor = prices.length > 0 ? Math.min(...prices) : 0;

    return {
      productId: p.id,
      name: p.name,
      category: p.category,
      description: p.description,
      priceMinor: priceMinor,
      images: p.images || [],
      variants: (p.variants || []).map(v => ({
        variantId: v.id,
        size: v.size,
        color: v.color,
        sku: v.sku,
        stockQuantity: v.quantity
      }))
    };
  });

  // 3. Process Voucher
  const rawVoucher = vouchersRaw[0] || {};
  const voucher = {
    code: rawVoucher.code || "WELCOME10",
    rate: (rawVoucher.discount_value || 10) / 100,
    maxDiscountMinor: rawVoucher.max_discount || 800000,
    minOrderMinor: rawVoucher.min_purchase || 1000000
  };

  // Compile into output format
  const result = {
    brandName: "Store Manage",
    accounts,
    customers,
    employees,
    products,
    voucher
  };

  const outputContent = `window.storefrontData = ${JSON.stringify(result, null, 2)};\n`;

  fs.writeFileSync(targetFile, outputContent, 'utf8');
  console.log(`Successfully synced and generated: ${targetFile}`);
} catch (err) {
  console.error("Failed to sync demo data:", err);
  process.exit(1);
}
